This directory includes my work during my summer internship at MSL.

#mutilate-master: Originated from https://github.com/leverich/mutilate.
  changes: 
	    1- new option "-N" to change the key distribution. Initially mutilate generates keys according to a uniform distribution. With this option, we can use any distribution. 
			2- new zipfian distribution: 
				example: -N zipfian:1.15,4000: create zipfian distribution with alpha=1.15 over 
                 a range of 4000 millin keys (1 -> 4 billion)
								 The zipfian distribution is approximate. It also generates a file for the
								 distribution that it can use for subsequent runs. If the file exists in the 
								 directory mutilate is ran, it doesn't generate the distribution again.
			           Note: When using this option, make sure you also set "-r {key range}".

			3- new option "-L {1,0}": By default mutilate does not load items if they happen to 
		 	   miss in Memcached. This option tells mutilate to send a reload the item every timei
				 it misses in Memcached. usage: "-L 1" default(0)

			4- new option "-n #records": Mutilate can set a bound on the duration of the test. This
			   option enables limiting the total number of requests. Remember that this option is 
				 per thread.

	 Examples of running mutilate can be found in memcached_tests directory.

#distributions: 
   This directory includes large files encoding zipfian distributions for various alpha
	 and range parameters. If these files are present where mutilate is run, mutilate can use
	 them instead of generating the distributions again.

#rd-trace: 
   This directory includes the source code for reuse distance analysis. The Makefile compiles
	 the code into a shared library (librd_trace.so), which must be linked into memcached to 
	 enable reuse distance profiling. Details about the reuse distance profiler can be found 
	 in the paper.
	 Note: During the last week of the internship we tailored our reuse distance profiler to
	 work in online profiler. So currently, the best way to use the profiler is to use in the
	 online analysis. To be able to run the profiler, we need to environment variables to be
	 set:  
	       PROF_LEN: length of the profiling window
				 WARMUP_LEN: lenght of the warmup stage (in number of requests)
				 make sure PROF_LEN >= WARMUP_LEN (in number of requests)

	 Previously, we have been using the RD_TRACE environment variable to figure out if we want
	 to profile or not. Currently, it always profiles the trace during the profiling window.


#memcached-1.4.25-rd: Originated from Memcached-1.4.25
    Memcached to use when doing the offline reuse distance analysis.
    changes: Several hooks are added in trace.h.
		   rd_trace_get: hook to reuse distance profiler for get requests.
			 rd_trace_set: hook to reuse distance profiler for set requests.
			 rd_trace_set_chunk_size: hook to get the chunk size for each slab clas
	  Please remember that the interface to rd_trace_get has been changed from keys to hash
		values only. So the first two parameters in rd_trace_get and rd_trace_set are not needed

#memcached-data: 
    This directory includes our data. Some of the data are the measured miss ratio for
		original Memcached. Some include the miss ratio predicted using the reuse distance
		analysis.
		example1: mrc-4BKey-800Mreq-exp0.000001.csv: The miss ratio curve predicted over 800 
		million requests from an exponential distribution with lambda=0.000001.
		example2: mrc-4Bkey-800Mreq-1.15alpha.csv: The miss ratio curve predicted over 800 
		million requests from a zipfian distribution over a range of 4 billion keys and with
		alpha=1.15.

#Rscripts:
    This directory includes our R scripts to generate nice graphs according to our data.

#memcached-dyn
     This diectory includes our Memcached version that can dynamically allocate and 
		 deallocate memory. Like memcached-1.4.25-rd, trace.h includes hooks to librd_trace.so.
		 The bulk of dynamic deallocation/allocation can be found in slab.c. The function 
		 slabs_deallocate is the entry point for deallocation. This function must be sorrounded
		 by calls to start_slab_monitor and stop_slabs_monitor.
		 Note: To find out about how memcached-dyn can be used please look for the call to 
		 slabs_deallocate in rd-trace.

#dyn-server
     This directory includes our dynamic provisioning server, which can potentially be used 
		 in a multi-tenant settting (several instances of Memcached), along with reuse distance
		 profiling (memcached-1.4.25-rd). Here is how we can run the multi-tenant environment:
		 
		 ./server.exe (Listens to port (9876) for incoming connection from Memcached applications)
		 
		 ./memcached-1.4.25-rd/memcached  (1st tenant)
		 (code in rd-trace lets Memcached to connect to the server. I believe that code is currently commented out or is not in the execution path).

		 ./memcached-1.4.25-rd/memcached (2nd tenant)

		 After some period server.exe asks for miss ratio curves from each tenants. Once it gets the results back, it uses lp_solve to compute an optimal solution. Currently, the optimal 
		 solution is one that gives the lowest overall average latency. We're still unsure what
		 the optimal solution should be in a multi-tenant environment. But this code gives us
		 ability to test different objective functions.

#lp-solve
    Sometimes we just want to use lp_solve to compute the solution for a particular resource
		provisioning problem. So rather than running memcached with profiling again, we can use
		this tool to find the lp solution. Remember to put the miss ratio curve inside MR.csv 
		before you run this tool. Currently, there directory includes one example. The tool
		then asks for the latency bound. (LP finds optimal cost for a latency bound). Give the
		desired latency in ms. The tool gives you the optimal resource provisioning
		configuration.

#memcached-tests
    This directory includes tests scripts to generate data. Typically each test script runs
		a memcached instance and then runs the workload generator with appropriate parameters.
		The scripts are self-explanatory.
	
#ElCached-Final-Takeout.zip
    This archive contains the source and diagrams for our paper submitted to INFLOW16.

Finally, if there is any question please ask me at rlavaee at cs dot rochester dot edu
