#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/wait.h>

#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <cstdio>
#include <unistd.h>
#include <map>
#include <pthread.h>
#include <iostream>
#include <atomic>
#include <mutex>
#include <thread>
#include <vector>

#include <lpsolve/lp_lib.h>

unsigned L1_size, L2_size;
float lat_th;

std::mutex serv_mutex;
int total_memcached_count = 0;
std::map<struct bufferevent *, int> memcached_id;

std::vector<std::vector <std::pair<unsigned, float>>> mrc;
std::vector<float> min_lats;


int prof_length = 300;


char * get_char_str(const std::string& str){
  char * writable = new char[str.size() + 1];
  std::copy(str.begin(), str.end(), writable);
  writable[str.size()] = '\0';
	return writable;
}

void gen_solve_lp(){
  lprec *lp;
	int Ncol, *colno = NULL, j, ret = 0;
	REAL *row = NULL;
	Ncol = mrc.size()*6; /* for each memcached, mr, size, latency & total latency, total cost */

	lp = make_lp(0, Ncol);

	if(lp == NULL)
		ret = 1; /* Couldn't construct a new model */

	if(ret==0) {
		std::string L1mr("mr.L1");
		std::string L2mr("mr.L2");
		std::string L1size("size.L1");
		std::string L2size("size.L2i");
		std::string lat("lat");
		std::string cost("cost");

		for(int i=0; i< mrc.size(); ++i){
			std::string i_str = "."+std::to_string(i);
			std::string L1size_i_str = L1size+i_str;
			std::string L2size_i_str = L2size+i_str;
			std::string L1mr_i_str = L1mr+i_str;
			std::string L2mr_i_str = L2mr+i_str;
			std::string lat_i_str = lat+i_str;
			std::string cost_i_str = cost+i_str;

	    set_col_name(lp, 6*i+1, get_char_str(L1size_i_str));
	    set_col_name(lp, 6*i+2, get_char_str(L2size_i_str));
	    set_col_name(lp, 6*i+3, get_char_str(L1mr_i_str));
	    set_col_name(lp, 6*i+4, get_char_str(L2mr_i_str));
			set_col_name(lp, 6*i+5, get_char_str(lat_i_str));
			set_col_name(lp, 6*i+6, get_char_str(cost_i_str));
	  }

		/*create space large enough for one row */

		colno = (int *) malloc(Ncol * sizeof(*colno));
		row = (REAL *) malloc(Ncol * sizeof(*row));

		if((colno==NULL) || (row == NULL))
			ret = 2;

	}

	std::cerr << ret << "\tcreated the model\n";

	if(ret == 0){
	  set_add_rowmode(lp, TRUE); /*makes building the model faster if it is done rows by row*/

		for(int i=0; i < mrc.size(); ++i){
		  for(int j=0 ; j < mrc[i].size() -1 ; ++j ){
				if(ret == 0){
				  auto p1 = mrc[i][j];
				  auto p2 = mrc[i][j+1];
					//std::cerr << "(" << p1.first << "," << p1.second << ")\t("<< p2.first << "," << p2.second <<")\n";
					{ 
			      int k = 0;
				    colno[k] = 6*i+1;
				    row[k++] = p1.second - p2.second;

				    colno[k] = 6*i+3;
				    row[k++] = p2.first - p1.first;

				    /* add the row to lpsolve */
				    if(!add_constraintex(lp, k, row, colno, GE, p1.second * p2.first - p1.first * p2.second))
					    ret = 3;
					}

					{ 
			      int k = 0;
 
						colno[k] = 6*i+2;
						row[k++] = p1.second - p2.second;

				    colno[k] = 6*i+4;
				    row[k++] = p2.first - p1.first;

				    /* add the row to lpsolve */
				    if(!add_constraintex(lp, k, row, colno, GE, p1.second * p2.first - p1.first * p2.second))
					    ret = 3;
					}



			  }
			}

			if(ret == 0){
			  int k = 0;
				colno[k] = 6*i+1;
				row[k++] = 1;

				if(!add_constraintex(lp, k, row, colno, GE, mrc[i].front().first))
						ret = 3;
			}

			if(ret == 0){
			  int k = 0;
				colno[k] = 6*i+2;
				row[k++] = 1;

				if(!add_constraintex(lp, k, row, colno, GE, mrc[i].front().first))
						ret = 3;
			}

			if(ret == 0){
			  int k = 0;
				colno[k] = 6*i+1;
				row[k++] = 1;

				if(!add_constraintex(lp, k, row, colno, LE, mrc[i].back().first))
						ret = 3;
			}

			if(ret == 0){
			  int k = 0;
				colno[k] = 6*i+2;
				row[k++] = 1;

				if(!add_constraintex(lp, k, row, colno, LE, mrc[i].back().first))
						ret = 3;
			}


			if(ret == 0){
			  int k = 0;
				colno[k] = 6*i+1;
				row[k++] = -1;

				colno[k] = 6*i+2;
				row[k++] = 1;
				
				if(!add_constraintex(lp, k, row, colno, GE, 0))
						ret = 3;
			}



			if(ret == 0){
			  int k = 0;
				colno[k] = 6*i+3; /* mr.L1 */
				row[k++] = 0.2 - 0.1; /* KVD lat - DRAM lat */

				colno[k] = 6*i+4; /* mr.L2 */
				row[k++] = 10-0.2; /*backend lat - KVD lat */

				colno[k] = 6*i+5; /* lat */
				row[k++] = -1;

				if(!add_constraintex(lp, k, row, colno, EQ, -0.1))
						ret = 3;
			}

		   min_lats.push_back(lat_th*(mrc[i].back().second*10 + (1-mrc[i].back().second)*0.1));

			/* especial for our workload latency <= 0.67 */

			if(ret == 0){
			  int k = 0;
				colno[k] = 6*i+5;
				row[k++] = 1;


				if(!add_constraintex(lp, k, row, colno, LE, min_lats[i]))
					ret = 3;
			}

		  if(ret == 0){
			  int k = 0;
				colno[k] = 6*i+6;
				row[k++] = -1;

				colno[k] = 6*i+1;
				row[k++] = (10.0-0.68) / 1024;

				colno[k] = 6*i+2;
				row[k++] = 0.68 / 1024;

				if(!add_constraintex(lp, k, row, colno, EQ, 0))
					ret = 3;
			}

		}
	}
	
	std::cerr << ret << "\tadded MR constraints\n";

	//if(ret == 0){

	//  /*total DRAM size constraint */
	//	int k = 0;
	//	for(int i=0; i<total_memcached_count; ++i){
	//	  colno[k] = 6*i+1;
	//		row[k++] = 1;
	//	}

	//	if(!add_constraintex(lp, k, row, colno, LE, 2*1024))
	//		ret = 3;

	//	/*total KVD size constraint */
	//	k = 0;
	//	for(int i=0; i<total_memcached_count; ++i){
	//	  colno[k] = 6*i+2;
	//		row[k++] = 1;
	//	}

	//	if(!add_constraintex(lp, k, row, colno, LE, 20*1024))
	//		ret = 3;

	//}

	//std::cerr << ret << "\tadded DRAM constraints\n";

	if(ret == 0){

		set_add_rowmode(lp, FALSE); /*rowmode should be turned off again when done building the model */

		/* for now, set the objective function to be the total cost */
		int k = 0;
		for(int i=0; i < mrc.size(); ++i){
		  colno[k] = 6*i+6;
			row[k++] = 1;
		}

		if(!set_obj_fnex(lp, k, row, colno))
			ret = 4;
	}


	if(ret == 0) {
		/* set the object direction to maximize */

		set_minim(lp);

		/* just out of curioucity, now show the model in lp format on screen */
		/* this only works if this is a console application. If not, use write_lp and a filename */
		/* write_LP(lp, stdout); */
		write_lp(lp, "model.lp");

		/* I only want to see important messages on screen while solving */
		set_verbose(lp, IMPORTANT);

		/* Now let lpsolve calculate a solution */
		ret = solve(lp);
		/*
		if(ret == OPTIMAL)
			ret = 0;
		else
			ret = 5;
			*/
	}

	std::cerr << ret << "\tobjective function set\tsolving:\n" ;

	if(ret == 0) {
		/* a solution is calculated, now lets get some results */

		/* objective value */
		fprintf(stderr,"Objective value: %f\n", get_objective(lp));
		fprintf(stderr,"latency bound: %f\n", min_lats[0]);

		/* variable values */
		get_variables(lp, row);
		for(j = 0; j < Ncol; j++)
			printf("%s: %f\n", get_col_name(lp, j + 1), row[j]);

		L1_size = (unsigned)ceil(row[0]);
		L2_size = (unsigned)ceil(row[1]);

		/* we are done now */
	}

	/* free allocated memory */
	if(row != NULL)
		free(row);
	if(colno != NULL)
		free(colno);

	if(lp != NULL) {
		/* clean up such that all used memory by lpsolve is freed */
		delete_lp(lp);
	}

}

std::map<struct bufferevent *, int> wait_for_mrc;

std::atomic_int mrc_computed(0);

static void set_tcp_no_delay(evutil_socket_t fd)
{
	int one = 1;
	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY,
			&one, sizeof one);
}

static void signal_cb(evutil_socket_t fd, short what, void *arg)
{
	struct event_base *base = (struct event_base *) arg;
	printf("stop\n");

	event_base_loopexit(base, NULL);
}

static void echo_read_cb(struct bufferevent *bev, void *ctx)
{
	/* This callback is invoked when there is data to read on bev. */
	struct evbuffer *input = bufferevent_get_input(bev);
	printf("reading %p \n",input);

	char buf[1024];
	int n;

	while ((n = evbuffer_remove(input, buf, sizeof(buf))) > 0){
		if(wait_for_mrc[bev]){
			int id = memcached_id[bev];
			std::cerr << "clear address: " << bev << "\t" << id <<"\n";
			mrc[id].clear();
		  uint32_t mrc_size = ntohl(*((uint32_t *) buf));
			char * ptr = buf + sizeof(uint32_t);
			printf("mrc size is : %u\n",mrc_size);
			for(int i=0; i<mrc_size; i++){
			  uint32_t mbh = ntohl(*((uint32_t *) ptr));
				ptr+= sizeof(uint32_t);
				uint32_t mr_i = ntohl(*((uint32_t *) ptr));
				float mrh = *((float *) (&mr_i));
				ptr+= sizeof(uint32_t);
				mrc[id].push_back(std::pair<uint32_t,float>(mbh,mrh));
			}

			for(auto kv: mrc[id]){
			  std::cerr << kv.first << "\t" << kv.second << "\n";
			}
			
	    std::unique_lock<std::mutex> lk(serv_mutex);
			mrc_computed++;
			std::cerr << "mrc_computed: " << mrc_computed << "\t" << total_memcached_count << "\n";
      if(mrc_computed==total_memcached_count){
				std::cerr << "generating and solving lp\n";
				gen_solve_lp();
				struct evbuffer *output = bufferevent_get_output(bev); 
				char * L1buf = (char *)malloc(sizeof(unsigned));
			  uint32_t L1_size_n = htonl(L1_size);
				memcpy(L1buf, (char*) (&L1_size_n), sizeof(unsigned));
	      evbuffer_add_printf(output, "change");
			  evbuffer_add(output, L1buf, sizeof(unsigned));

				mrc_computed = 0;
			}
			lk.unlock();

		}
	  //fwrite(buf, 1, n, stdout);
	}


	//struct evbuffer *output = bufferevent_get_output(bev);

	/* Copy all the data from the input buffer to the output buffer. */
	//evbuffer_add_buffer(output, input);
}

static void echo_event_cb(struct bufferevent *bev, short events, void *ctx)
{
	struct evbuffer *output = bufferevent_get_output(bev);
	size_t remain = evbuffer_get_length(output);
	if (events & BEV_EVENT_ERROR) {
		perror("Error from bufferevent");
	}

	if(events & BEV_EVENT_EOF){
	  printf("End of file\n");
	}
	if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
		printf("closing, remain %zd\n", remain);
		bufferevent_free(bev);
	}
}

pthread_t trace_pid;

void * compute_mrc_loop(void * arg){
 
	bufferevent *bev = (bufferevent *) arg;
	while(1){
	  sleep(120);
		wait_for_mrc[bev] = 0;
	  evbuffer_add_printf(bufferevent_get_output(bev), "compute_mrc");
	  wait_for_mrc[bev] = 1;
	}

	return NULL;
}

void * trace_thread(void *arg){
  bufferevent * bev = (bufferevent *) arg;
	unsigned warmup_length = prof_length/10;
	evbuffer_add_printf(bufferevent_get_output(bev),"start_warmup");
	
	pid_t pID = fork();

	if(pID == 0){ //child
	   execl("/home/rlavaee/mutilate-master/mutilate","/home/rlavaee/mutilate-master/mutilate ","--noload","--binary","-s","127.0.0.1","-K","fb_key","-V","fb_value","-i","fb_ia","-T","4","-N","zipfian:1.15,4000","-r","4000","-t","30000","-L","1","-n","200000000",(char *)0);
	}else if(pID < 0){
	   fprintf(stderr,"failed to fork!\n");
		 exit(-1);
	}else{
	  fprintf(stderr,"parent process\n");
		sleep(warmup_length);
	  evbuffer_add_printf(bufferevent_get_output(bev), "start_tracing");
	  sleep(prof_length);
		wait_for_mrc[bev] = 0;
	  evbuffer_add_printf(bufferevent_get_output(bev), "stop_tracing");
	  wait_for_mrc[bev] = 1;
		int status;
		pid_t _pid = wait(&status);
		fprintf(stderr,"mutilate is done\n");
		exit(0);
	}

}

static void accept_conn_cb(struct evconnlistener *listener,
		evutil_socket_t fd, struct sockaddr *address, int socklen,
		void *ctx)
{
	printf("got new connection\n");

	struct sockaddr_in * address_in = (struct sockaddr_in *) address;

	 printf("socket address: 0x%x\n, socket port: 0x%x\n",address_in->sin_addr.s_addr,address_in->sin_port);
	/* We got a new connection! Set up a bufferevent for it. */
	struct event_base *base = evconnlistener_get_base(listener);
	struct bufferevent *bev = bufferevent_socket_new(
			base, fd, BEV_OPT_CLOSE_ON_FREE);
	set_tcp_no_delay(fd);

	bufferevent_setcb(bev, echo_read_cb, NULL, echo_event_cb, NULL);

	bufferevent_enable(bev, EV_READ|EV_WRITE);

  std::unique_lock<std::mutex> lk(serv_mutex);	
	std::cerr << "address: " << bev << "\t" << total_memcached_count << "\n";
	memcached_id[bev]=total_memcached_count++;
	mrc.resize(total_memcached_count);
	lk.unlock();

	pthread_create(&trace_pid, NULL, trace_thread, (void *)bev);

	
	 // pthread_create(&compute_mrc_pid, NULL, compute_mrc_loop, (void *)bev);


}

int main(int argc, char **argv)
{
	struct event_base *base;
	struct evconnlistener *listener;
	struct sockaddr_in sin;
	struct event *evstop;

	int port = 9876;

	int opt;

	while((opt = getopt(argc, argv, "p:l:t:"))!= -1){
	  switch(opt){
		  case 'p': 
				port = atoi(optarg); 
				break;
			case 'l': 
				prof_length = atoi(optarg); 
				break;
			case 't':
				lat_th = atof(optarg);
				break;
			default:
				abort();
		}
	}

	if (port<=0 || port>65535) {
		puts("Invalid port");
		exit(-1);
	}

	signal(SIGPIPE, SIG_IGN);

	base = event_base_new();
	if (!base) {
		puts("Couldn't open event base");
		return 1;
	}

	evstop = evsignal_new(base, SIGHUP, signal_cb, base);
	evsignal_add(evstop, NULL);

	/* Clear the sockaddr before using it, in case there are extra
	 *    *          * platform-specific fields that can mess us up. */
	memset(&sin, 0, sizeof(sin));
	/* This is an INET address */
	sin.sin_family = AF_INET;
	/* Listen on 0.0.0.0 */
	sin.sin_addr.s_addr = htonl(0);
	/* Listen on the given port. */
	sin.sin_port = htons(port);

	listener = evconnlistener_new_bind(base, accept_conn_cb, NULL,
			LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1,
			(struct sockaddr*)&sin, sizeof(sin));
	if (!listener) {
		perror("Couldn't create listener");
		return 1;
	}

	event_base_dispatch(base);


	evconnlistener_free(listener);
	event_free(evstop);
	event_base_free(base);
	return 0;
}
