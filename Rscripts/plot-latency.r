#!/usr/bin/env Rscript
library(ggplot2)
library(reshape2)
library(plyr)
library(gridExtra)

zipf <- read.table(file.path("..","mrc-4Bkey-50Mreq-1.15alpha.csv"),sep="\t",col.names=c("size","mr"))
exp <- read.table(file.path("..","mrc-4Bkey-50Mreq-exp0.00001.csv"),sep="\t",col.names=c("size","mr"))
zipf[,"workload"] <- c("zipf-1.15-4Bkey-50Mreq")
exp[,"workload"] <- c("exp-0.00001-4Bkey-50Mreq")

df <- rbind(zipf,exp)
df$latency = df$mr*10/100 + (1-df$mr)*0.1/100

pdf(file.path("results","latency.pdf"))
ggplot(data=df, aes(x=size,y=latency,group=workload)) + geom_point(aes(colour=workload,shape=workload))+
   theme(legend.position="top")+
	 labs(x= "allocation (MB)", y="latency (ms)", title="latency curve based on miss ratio")
dev.off()


