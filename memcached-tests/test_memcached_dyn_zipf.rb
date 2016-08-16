#!/usr/bin/env ruby
require 'open3'
#mem_a = [32,64,96,128,160,192,224,256,320,384,448,512] 
#mem_a = [32,4I0,48,56,64,96,128,160,192,224]
raise("Usage: ./test_memcached_dyn.rb {mem}") if(ARGV.size != 1)
mem = ARGV[0]

trace_len = 800*1000*1000


warmup_len = trace_len/10

prof_lens = (warmup_len*6..trace_len).step(warmup_len*2).to_a

#prof_lens = [warmup_len*2]


prof_lens.each do |prof_len|

   stdin, stdout, stderr, wait_th = Open3.popen3({"PROF_LEN" => prof_len.to_s, "WARMUP_LEN" => warmup_len.to_s}, "/home/rlavaee/memcached-dyn/memcached  -m #{mem} -o slab_reassign")

	 sleep(2);

	 total_trace_len = (prof_len +trace_len)/4

	
	 Open3.popen3("./mutilate --noload --binary -s 127.0.0.1 -K fb_key -V fb_value -i fb_ia -T 4  -N zipfian:1.15,4000 -r 4000 -t 30000 -n #{total_trace_len} -L 1") do |_stdin, _stdout, _stderr, _wait_th|

	 pid = _wait_th.pid
   exit_status = _wait_th.value 

	filename = "zipf.step#{prof_len}.mem#{mem}.lat1.01.out"

   slabs = `echo stats slabs | nc 127.0.0.1 11211`
   stats = `echo stats | nc 127.0.0.1 11211`

	 begin
	   Process.kill(:KILL,wait_th.pid)
   rescue Exception => e
		 puts e.message
	 end

	 File.open(filename,"w") do |f|

	     until (line = _stdout.gets).nil? do
				 f.puts(line)
		   end

		   f.puts "_____________________"
			 

	     until (line = _stderr.gets).nil? do
	       f.puts(line)
		   end



		   f.puts "_____________________"

	     until (line = stdout.gets).nil? do
				 f.puts(line)
		   end

		   f.puts "_____________________"
			 

	     until (line = stderr.gets).nil? do
	       f.puts(line)
		   end

			 f.puts "---------------------"
			 f.puts stats

			 f.puts "---------------------"
			 f.puts slabs



	   end
	 end



end

