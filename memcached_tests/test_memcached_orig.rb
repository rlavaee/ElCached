#!/usr/bin/env ruby
require 'open3'
#mem_a = [32,64,96,128,160,192,224,256,320,384,448,512] 
#mem_a = [32,4I0,48,56,64,96,128,160,192,224]
raise("Usage: ./test_memcached_orig.rb {mem}") if(ARGV.size != 1)
mem = ARGV[0]

filename = "mr-orig.csv"

File.open(filename,"w") {|f| }




#stdin, stdout, stderr, wait_th = Open3.popen3("/home/rlavaee/memcached-1.4.25-rd/memcached  -m #{mem}")
stdin, stdout, stderr, wait_th = Open3.popen3("/home/rlavaee/memcached-1.4.25-orig/memcached  -m #{mem}")

puts wait_th.pid

sleep(2)

_stdin, _stdout, _stderr, _wait_th = Open3.popen3("./mutilate --noload --binary -s 127.0.0.1 -K fb_key -V fb_value -i fb_ia -T 4 -N zipfian:1.15,4000 -r 4000 -t 30000 -L 1 -n 200000000")

puts _wait_th.pid

phase = 0
old_gets = gets = 0

loop do
	sleep(10);
	miss_stats = `echo stats | nc 127.0.0.1 11211`

	miss_stats.each_line do |line|
		gets = line[/\d+/].to_i if(line.include?("cmd_get"))
	end

	puts("#{phase}\t#{gets-old_gets}")


	File.open(filename,"a") do |file| 
		file.puts("#{phase}\t#{gets-old_gets}")
	end
	phase+=1
	old_gets = gets
end

