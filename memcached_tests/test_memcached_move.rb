#!/usr/bin/env ruby
require 'open3'
#mem_a = [32,64,96,128,160,192,224,256,320,384,448,512] 
#mem_a = [32,4I0,48,56,64,96,128,160,192,224]
#mem_a = [64,128, 256, 512, 1024, 2048, 4096, 8192,16384]
raise("Usage: ./test_memcached.rb {mem}") if(ARGV.size != 1)
mem = ARGV[0]

filename = "phase-mr-800Mreq-zipf-oldest.csv"

File.open(filename,"w") {|f| }


misses = old_misses = 0
gets = old_gets = 0

memory = 0



	   stdin, stdout, stderr, wait_th = Open3.popen3("/home/rlavaee/memcached-move/memcached -m #{mem} -o slab_reassign")

		 puts "memcached: #{wait_th.pid}"

	sleep(2)

    _stdin, _stdout, _stderr, _wait_th = Open3.popen3("./mutilate --noload --binary -s 127.0.0.1 -K fb_key -V fb_value -i fb_ia -T 4 -N zipfian:1.15,4000 -r 4000 -t 30000 -L 1 -n 200000000")

	   puts "mutilate: #{_wait_th.pid}"

	   loop do
			 sleep(2);
			 miss_stats = `echo stats | nc 127.0.0.1 11211`
		   slab_stats = `echo stats slabs | nc 127.0.0.1 11211`

			 miss_stats.each_line do |line|
				 misses = line[/\d+/].to_i if(line.include?("get_misses"))
				 gets = line[/\d+/].to_i if(line.include?("cmd_get"))
			 end

		   slab_stats.each_line do |line|
				 memory = line[/\d+/].to_i/(1024*1024) if(line.include?("total_malloced"))
		   end


			 phase_mr = (misses-old_misses).to_f / (gets-old_gets)

			 puts("#{memory}MB\t#{'%.3f' % phase_mr}\t#{gets-old_gets}\t#{misses-old_misses}")


		   File.open(filename,"a") do |file| 
			   file.puts("#{memory}MB\t#{'%.3f' % phase_mr}\t#{gets-old_gets}\t#{misses-old_misses}")
		   end
			 old_misses = misses
			 old_gets = gets
		end
