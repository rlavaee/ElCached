#!/usr/bin/env Rscript
library(reshape2)
library(ggplot2)
online <- read.table(file.path("..","memcached-data","online.csv"),sep="\t",header=TRUE)

online$measured.latency = (1-online$mr.L1.measured)*0.1 + (online$mr.L2.measured - online$mr.L1.measured)*0.2 + online$mr.L2.measured*10
online$size.L1.G = online$size.L1 / 1024
online$size.L2.G = (online$size.L2-online$size.L1) / 1024

online$latency.error = (online$latency - online$measured.latency)/ online$measured.latency


online$cost = online$size.L1.G * 10 + online$size.L2.G * 0.68

online.m <- melt(online,id=c("profiling"))
print(online)


pdf(file="online.pdf",width=4,height=3)
ggplot(data = online.m, aes(x=profiling, y=value, group=variable))+
   geom_bar(stat="identity", position="dodge",aes(color=variable))
dev.off()
