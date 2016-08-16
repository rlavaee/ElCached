#include "RedBlackTree.h"
#include <memory>
#include <cstdio> 
#include <string>
#include <unordered_map>
#include <map>
#include <cstring>
#include <cassert>
#include <pthread.h>
//#include <boost/lockfree/spsc_queue.hpp>
#include <boost/lockfree/queue.hpp>
#include <condition_variable>
#include <atomic>
#include <queue>

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include <sys/socket.h>
#include <netinet/tcp.h>

#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>

#include "LP.h"

#define MAX_COUNT 100000

unsigned prof_len, warmup_len;

void start_warmup();
void start_tracing();
void stop_tracing();

extern "C" size_t get_mem_malloced();
extern "C" int slabs_deallocate(const size_t); 

extern "C" void stats_reset(void);
extern "C" void start_slab_monitor();
extern "C" void stop_slab_monitor();

/* callback to memcached: get the total number of chunks allocated for a slab class 
 *   (allocated slabs) * (#chunks perslab) - (free chunks)*/
extern "C" unsigned get_chunk_count(unsigned slab_clsid);

/* callback to memcached: get the total number of slabs allocated for a slab class */
extern "C" unsigned get_slab_count(unsigned slab_clsid);

extern "C" void trigger_migration(unsigned new_mem_limit);



struct hashing_func {
	uint64_t operator()(const uint64_t key) const {
		return key;
	}
};

void compute_mrc();

enum req_type {
  get,
	set,
	del,
	update
};

class Request {
	public:
	uint64_t hv;
	req_type type;
	unsigned new_clsid;

	Request(){}

	Request(const Request& req){
		hv = req.hv;
		type = req.type;
		new_clsid = req.new_clsid;
	}

	Request(uint64_t _hv, unsigned _clsid, req_type _type){
    hv = _hv;
		new_clsid = _clsid;
		type = _type;
	}

	void profile(){
		switch(type){
		  case get:
				profile_get();
				break;
		  case set:
				profile_set();
				break;
			default:
				std::cerr << "no profiler registered for this request type\n";
				break;
		}
	}

	void profile_set();
	void profile_get();

};

boost::lockfree::queue <Request> reque(MAX_COUNT);
//std::atomic<bool> done(false);
std::atomic<bool> enable_trace(false);
std::atomic<bool> enable_measure(false);
std::atomic_int consumer_count(0);
std::atomic_int producer_count(0);
std::mutex reque_mutex;
std::mutex hist_mutex;
std::condition_variable reque_empty;
std::condition_variable reque_full;
unsigned long reque_count = 0;


pthread_t consume_reque_pid;
pthread_t mrc_event_pid;



bool triggered_once = false;

/* TODO: this needs to be consistent with memcached */
#define MAX_SLABS 64

/* Chunk size for every slab class */
unsigned chunk_size[MAX_SLABS]={};

/* Number of allocated chunks */
unsigned chunk_count[MAX_SLABS]={};

/* Number of allocated slabs for each slab class (we assume that the slab size is 1MB) */
unsigned slab_count[MAX_SLABS]={};

/* An rd_tree (reuse distance tree) for every slab class */
RedBlackTree * rd_tree[MAX_SLABS];


//int enable_trace = 0;
unsigned ltime = 0;  /* universal logical time */
int get_count = 0; /* sequence number of the get request */

std::unordered_map <uint64_t, std::pair<unsigned,unsigned>, hashing_func> keyNodeMap;
//std::unordered_map <std::string, unsigned> item_clsid; /* current slab class of the key */
std::vector<unsigned> rd_hist[MAX_SLABS]; /* reuse distance histograms */

std::map <unsigned, float> mrc; /* miss ratio curve */

/* LOCK for accessing data 
 * TODO: do finer grain locking, or no locking at all (lock-free queue) */
//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void reque_push(const Request& req){
	/*
  std::unique_lock<std::mutex> lk(reque_mutex);
	while(reque_count==MAX_COUNT)
		reque_full.wait(lk);
	//std::cerr << "finished waiting\t" << producer_count.load() << "\t" << consumer_count.load() << "\n";
	reque.push(req);
	reque_count++;
	lk.unlock();
	reque_empty.notify_one();
	*/
	while(!reque.push(req));
	++producer_count;
}


/* profiler for set requests: given key, nkey, and the slab class id,
 * we update the reuse distance tree, without updating the reuse distance
 * histogram */
extern "C" void rd_trace_set(uint64_t hv, unsigned new_clsid){
	//std::string str(key,nkey);
	//printf("w,%zu,%s\n",nkey,str.c_str());

	if(!enable_trace)
		return;

  Request req(hv,new_clsid,set);
	reque_push(req);
	
}

void Request::profile_set(){

	//ltime++;


	unsigned node = 0;
	auto res_node  = keyNodeMap.emplace(std::piecewise_construct,
																						std::forward_as_tuple(hv),
																						std::forward_as_tuple(new_clsid,node));

	if(!res_node.second){
		/* key was stored before: remove the old record from rd_tree and insert the new
		 * one in */
		unsigned old_clsid = res_node.first->second.first;

		rd_tree[old_clsid]->DeleteNode(res_node.first->second.second);


		/* update slab class id */
		res_node.first->second.first = new_clsid;
		//node = res_node.first->second.second;
	}
	//else{
		//res_node.first->second.second = node = new RedBlackTreeNode(new RedBlackEntry(ltime));
	//  res_node.first->second.second = node;
	//}


  node = rd_tree[new_clsid]->AddNode();
	res_node.first->second.second = node;
	/* update the last item */
  rd_tree[new_clsid]->InsertTail(node);

}

/* profiler for get requests: given key and nkey, we first find the key's 
 * slab class and then
 * update the corresponding reuse distance tree and reuse distance histogram */
extern "C" void rd_trace_get(uint64_t hv){
	if(!enable_trace)
		return;

	Request req(hv,0, get);
	reque_push(req);
}

void Request::profile_get(){

	ltime++;
	//fprintf(stderr,"LTIME: %u\n",ltime);

	if(ltime == warmup_len){
	  start_tracing();
	}else	if(ltime == prof_len){
		stop_tracing();
		return;
	}


	/* update get sequence number */
	if(enable_measure)
	  get_count++;
	

	auto res_node = keyNodeMap.find(hv);

	/* If key was never stored before, bail out. The item will not be inserted 
	 * in the tree */
	if(res_node != keyNodeMap.end()){
		/* just an assertion to ensure the consistency of item_clsid and last_access */
		unsigned clsid = res_node->second.first;
		unsigned rd = rd_tree[clsid]->GetDistance(res_node->second.second);


		/* update the reuse distance histogram */
		if(enable_measure){
		  if(rd_hist[clsid].size() <= rd)
			  rd_hist[clsid].resize((rd+1)*2);
		  rd_hist[clsid][rd]++;
		}


		rd_tree[clsid]->DeleteNode(res_node->second.second);
		res_node->second.second = rd_tree[clsid]->AddNode();
		rd_tree[clsid]->InsertTail(res_node->second.second);
	}
	
} 

int64_t total_bytes_read = 0;
int64_t total_messages_read = 0;

static void readcb(struct bufferevent *bev, void *ptr)
{
	fprintf(stderr,"has something to read\n");
	/* This callback is invoked when there is data to read on bev. */
	struct evbuffer *input = bufferevent_get_input(bev);
	struct evbuffer *output = bufferevent_get_output(bev);

	char buf[10000];
	int n;

	while ((n = evbuffer_remove(input, buf, sizeof(buf))) > 0){
	  fwrite(buf, 1, n, stdout);
		if(memcmp(buf,"change",6)==0){
			unsigned L1size = ntohl(*(unsigned *) (buf+6));
			fprintf(stderr,"L1 size is: %u\n",L1size);
			start_slab_monitor();
			sleep(10);
			struct timespec tstart={0,0}, tend={0,0};
			clock_gettime(CLOCK_MONOTONIC, &tstart);
			slabs_deallocate(L1size << 20);
			clock_gettime(CLOCK_MONOTONIC, &tend);
			double seconds = ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec);
			fprintf(stderr,"time to reach new mem_limit: %.5f\n", seconds);
			stop_slab_monitor();

			stats_reset();

		}else if(memcmp(buf,"stop_tracing",12)==0){
      fprintf(stderr,"HERE: STOPPING TRACE\n");		
			stop_tracing();
	    //std::unique_lock<std::mutex> lk(hist_mutex);
		  //compute_mrc();
			//lk.unlock();

			size_t mrcbuf_size = sizeof(uint32_t) + mrc.size() * 2 * sizeof(uint32_t);

			char * mrcbuf = (char *)malloc(mrcbuf_size);


			char * ptr = mrcbuf;

			uint32_t mrc_size = htonl(mrc.size());

			memcpy(ptr, (char*) (&mrc_size), sizeof(uint32_t));

			ptr+= sizeof(uint32_t);

			for(auto kv: mrc){
			  uint32_t mb =  kv.first;
				uint32_t * mr_i = (uint32_t *) (&kv.second);
				fprintf(stderr,"%u\t%f\n",mb,kv.second);

				uint32_t mbn=htonl(mb);
				uint32_t mrn=htonl(*mr_i);

				memcpy(ptr, (char*) (&mbn), sizeof(uint32_t));
				ptr+=sizeof(uint32_t);
				memcpy(ptr, (char*) (&mrn), sizeof(uint32_t));
				ptr+=sizeof(uint32_t);
		  }

			evbuffer_add(output, mrcbuf, mrcbuf_size);

		}else if(memcmp(buf,"start_warmup",12)==0)
			start_warmup();
		else if(memcmp(buf,"start_tracing",13)==0)
			start_tracing();

	}

	++total_messages_read;
	total_bytes_read += evbuffer_get_length(input);

	/* Copy all the data from the input buffer to the output buffer. */
	//evbuffer_add_buffer(output, input);
}

static void set_tcp_no_delay(evutil_socket_t fd)
{
	  int one = 1;
		  setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
					      &one, sizeof one);
}

static void eventcb(struct bufferevent *bev, short events, void *ptr)
{
	if (events & BEV_EVENT_CONNECTED) {
		evutil_socket_t fd = bufferevent_getfd(bev);
		set_tcp_no_delay(fd);
	} else if (events & BEV_EVENT_ERROR) {
		printf("NOT Connected\n");
	} else if (events & BEV_EVENT_EOF) {
	  printf("closing\n");
		struct event_base *base = (struct event_base *) ptr;

		bufferevent_free(bev);
		event_base_loopexit(base, NULL);

		printf("%zd total bytes read\n", total_bytes_read);
		
		printf("%zd total messages read\n", total_messages_read);


	}
}



void * mrc_event_loop(void *){
    struct event_base *base;
		struct bufferevent *bev;
		struct sockaddr_in sin;

		int port = 9876;

		base = event_base_new();

		if(!base) {
		  perror("couldn't open event base");
			exit(-1);
		}

		memset(&sin, 0, sizeof(sin));

		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = htonl(0x7f000001); /* 127.0.0.1 */
		sin.sin_port = htons(port);

	  bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);

		bufferevent_setcb(bev, readcb, NULL, eventcb, base);
		bufferevent_enable(bev, EV_READ|EV_WRITE);

		fprintf(stderr,"printing hello\n");

		//evbuffer_add_printf(bufferevent_get_output(bev), "client: hello\r\n");

		//evbuffer_add(bufferevent_get_output(bev), message, 6);

		if (bufferevent_socket_connect(bev,
						(struct sockaddr *)&sin, sizeof(sin)) < 0){
		    /* Error starting connectin */
			perror("error connecting");
			exit(-1);
		}
		event_base_dispatch(base);

		return NULL;

}

void* consume_reque(void*){
	Request req;
	sleep(2);
  while(enable_trace || !reque.empty()){
		while(reque.pop(req)){
			if(enable_trace)
			  req.profile();
			consumer_count++;
		}
		//std::cerr << "producer: " << producer_count.load() << "\tconsumer: " << consumer_count.load() << "\tdone: "<< done << "\n";
		/*
	  std::unique_lock<std::mutex> lk(reque_mutex);
		while(!reque_count){
		  reque_empty.wait(lk);
		}
		//std::cerr << "empty waiting finished\n";
		req = reque.front();
		reque.pop();
		reque_count--;
		lk.unlock();
		reque_full.notify_one();
		req.profile();
		++consumer_count;
		*/
	}

	compute_mrc();

	for(unsigned id=0; id<MAX_SLABS;++id){
		delete rd_tree[id];
		rd_hist[id].clear();
	}

	keyNodeMap.clear();


	  unsigned L1_size, L2_size;
	 std::vector< std::vector< std::pair<unsigned, float> > > vmrc(1);
		
	  for(auto kv: mrc)
		  vmrc[0].push_back(kv);
		if(gen_solve_lp(vmrc,L1_size,L2_size) != 0){
		   fprintf(stderr,"LP infeasible\t exiting!\n");
			 exit(-1);
		}
	  fprintf(stderr,"L1 size is: %u\n",L1_size);
    start_slab_monitor();
		sleep(10);
    struct timespec tstart={0,0}, tend={0,0};
			clock_gettime(CLOCK_MONOTONIC, &tstart);

		slabs_deallocate(L1_size << 20);
			clock_gettime(CLOCK_MONOTONIC, &tend);
			double seconds = ((double)tend.tv_sec + 1.0e-9*tend.tv_nsec) - ((double)tstart.tv_sec + 1.0e-9*tstart.tv_nsec);
			fprintf(stderr,"time to reach new mem_limit: %.5f\n", seconds);

		stop_slab_monitor();
		stats_reset();

		//std::cerr << "producer: " << producer_count.load() << "\tconsumer: " << consumer_count.load() << "\tdone: "<< done << "\n";

	return NULL;

}

void start_warmup(){
	//fprintf(stderr,"starting_trace\n");

	for(unsigned id=0; id<MAX_SLABS;++id)
		rd_tree[id] = new RedBlackTree();

	enable_measure.store(false);
  enable_trace.store(true);

  pthread_create(&consume_reque_pid, NULL, consume_reque, NULL);
}

void start_tracing(){
	fprintf(stderr,"starting measure\n");
  enable_measure.store(true);
}

void stop_tracing(){
	fprintf(stderr,"stopping trace\n");
	enable_trace.store(false);
	//pthread_join(consume_reque_pid, NULL);
}

__attribute__((constructor))
	void set_enable_trace(){
		/*
		const char * enable_trace_str = getenv("RD_TRACE");
		enable_trace = (enable_trace_str)?((strcmp(enable_trace_str,"1")==0)?(1):(0)):(0);
		fprintf(stderr,"trace enabled: %d\n",enable_trace);

		if(!enable_trace)
			return;
			*/


    //pthread_create(&mrc_event_pid, NULL, mrc_event_loop, NULL);

		mallopt(M_TRIM_THRESHOLD, 4 << 10);
		
		const char * prof_len_str = getenv("PROF_LEN");
		if(prof_len_str == NULL){
			std::cerr << "PROF_LEN unset!\n";
			exit(-1);
		}
		prof_len = atoi(prof_len_str);

		const char * warmup_len_str = getenv("WARMUP_LEN");
		if(warmup_len_str == NULL){
			std::cerr << "WARMUP_LEN unset!\n";
			exit(-1);
		}
		warmup_len = atoi(warmup_len_str);

		assert((warmup_len < prof_len) && "warmup len not smaller than prof len");

		enable_trace.store(false);
		start_warmup();
		
	}


//__attribute__((destructor))
	void compute_mrc (){
		
		size_t mem_malloced = get_mem_malloced();
		fprintf(stderr,"mem_malloced: %zu\n",mem_malloced);

		mrc.clear();

		/*
		if(!SearchStats::searches)
			return;

		fprintf(stderr,"average search hops: %f\n",((float)(SearchStats::search_depth_sum))/SearchStats::searches);
		*/

		/* total allocation in bytes */
		unsigned total_alloc = 0;
		
		/* total slabs allocated */
		unsigned total_slabs = 0;

		for(unsigned id=0; id<MAX_SLABS;++id){
			chunk_count[id]=get_chunk_count(id);
			slab_count[id]=get_slab_count(id);
			total_slabs+=slab_count[id];
			total_alloc+=chunk_count[id]*chunk_size[id];
		}
		
		/* iterators for running over the reuse distance histograms */
		//std::map <unsigned,unsigned>::iterator iters[MAX_SLABS];
		unsigned index[MAX_SLABS];

		/* initialize iterators to the beginning of the histograms */
		for(auto id=0; id<MAX_SLABS; ++id)
			index[id]=0;

		/* number of total (get) misses, initialized by the total gets */
		unsigned misses = get_count;

		/* cache size in MBs */
		unsigned cache_size_base, cache_size_power;
		cache_size_power = 32;

		/* initalize cache_size_power to the first power of 2, not smaller than total_slabs */
		//while(cache_size_power < total_slabs) { cache_size_power <<= 1; };

		cache_size_base = cache_size_power;

		/* number of slab classes with all requests hit */
		int finished;

		do{
			finished = 0;
			/* actual number of slabs allocated (real cache size) */
			unsigned cache_size = 0;

			std::cerr << "cache size base: " << cache_size_base << "\n";

			for(auto id=0; id<MAX_SLABS; ++id){
				if(chunk_size[id]!=0){

					unsigned alloc_slabs = std::max<unsigned>(cache_size_base * slab_count[id] / total_slabs,1);
					cache_size += alloc_slabs;
				  unsigned cap = (alloc_slabs << 20) / chunk_size[id];
				  while(index[id]!=rd_hist[id].size() && index[id] <= cap){
					  misses -= rd_hist[id][index[id]];
					  index[id]++;
				  }
				  if(index[id]==rd_hist[id].size())
					  finished++;
				}else
					finished++;
			}


			
			mrc[cache_size]=(float)misses/get_count;

			if(cache_size_base == cache_size_power){
			   cache_size_power <<= 1;
			}
			cache_size_base += (cache_size_power >> 2);

		}while(finished!=MAX_SLABS);

		/* print the miss ratio curve */

		for(auto kv: mrc)
			fprintf(stderr,"%u\t%f\n",kv.first,kv.second);
	}

/* data initializer for a slab class (must be called from memcached)*/
extern "C" void rd_trace_set_chunk_size(unsigned slabs_clsid, unsigned size){
	chunk_size[slabs_clsid] = size;
}
