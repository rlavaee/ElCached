#!/usr/bin/env ruby
require 'open3'
#mem_a = [32,64,96,128,160,192,224,256,320,384,448,512] 
#mem_a = [32,4I0,48,56,64,96,128,160,192,224]
mem_a = [64,128, 256, 512, 1024, 2048, 4096, 8192,16384]
raise("Usage: ./test_memcached.rb {alpha}") if(ARGV.size != 1)
alpha = ARGV[0]

filename = "mr-measure-800Mreq-exp#{alpha}.csv"

File.open(filename,"w") {|f| }


mem_a.each do |mem| 


	   stdin, stdout, stderr, wait_th = Open3.popen3("/home/rlavaee/memcached-1.4.25-rd/memcached  -m #{mem}")

		 puts wait_th.pid

	sleep(5)

	Open3.popen3("./mutilate --noload --binary -s 127.0.0.1 -K fb_key -V fb_value -i fb_ia -T 4  -N zipfian:#{alpha},4000 -r 4000 -t 30000 -n 200000000 -L 1") do |_stdin, _stdout, _stderr, _wait_th|
	#Open3.popen3("./mutilate --noload --binary -s 127.0.0.1 -K fb_key -V fb_value -i fb_ia -T 4  -N exponential:#{alpha} -t 30000 -n 200000000 -L 1") do |_stdin, _stdout, _stderr, _wait_th|
	   _pid = _wait_th.pid
	   until (line = _stdout.gets).nil? do
	      puts line 
	      if(line.start_with?("Misses"))
		       misses = line[/\d+\.\d+/].to_f 
	      end
	   end
		 memcached_stats = `echo stats slabs | nc 127.0.0.1 11211`
		 puts wait_th.pid
	   Process.kill(:KILL,wait_th.pid)

		 memory = mem

		 memcached_stats.each_line do |line|
				if(line.include?("total_malloced"))
					memory = line[/\d+/].to_i/(1024*1024)
				end
		 end
		 File.open(filename,"a") do |file| 
			 file.puts("#{memory}\t#{misses}")
		 end
  end
end
