#!/usr/bin/env Rscript
library(ggplot2)
library(reshape2)
library(plyr)
library(gridExtra)
library(grid)



fetch_workload_data <- function(path) { 
  workload <- read.table(path,sep="\t",col.names=c("size","mr"))
  workload$mr <- workload$mr/100

  t <- tail(workload,n=1)
  workload <- rbind(workload, c(t[1,1]*30,t[1,2]))
  workload <- rbind(workload, c(0,1))


  workload.approx = approx(workload$size, workload$mr, xout = c(seq(0,3072,32)))
  workload.approx.power = approx(workload$size, workload$mr, xout=workload.approx$x*5)

	df = data.frame(size.L1=workload.approx$x, size.L2i=workload.approx.power$x, mr.L1 = workload.approx$y, mr.L2 = workload.approx.power$y)


  df$size.L2e = df$size.L2i - df$size.L1

  df$cost = df$size.L1*10/1024 + df$size.L2e*0.68/1024
  df$latency = df$mr.L2*10+(df$mr.L1-df$mr.L2)*0.2 + (1-df$mr.L1)*0.1

  #df$latency = ceiling(df$latency*100)/100

	return(df)
}

#zipf_file <- file.path("..","memcached-data","mrc-4Bkey-800Mreq-1.15alpha.csv")
#exp_file <- file.path("..","memcached-data","mrc-800Mreq-pareto-scale20-shape5.csv")

zipf_file <- file.path("..","memcached-data","mrc-4Bkey-800Mreq-1.15alpha.csv")
exp_file <- file.path("..","memcached-data","mrc-4Bkey-800Mreq-exp0.000001.csv")

files <- c(zipf_file,exp_file)

workloads <- lapply(files, FUN = fetch_workload_data)

zipf_w <- workloads[[1]]
zipf_w$workload <- "Tenant 1"
exp_w <- workloads[[2]]
exp_w$workload <- "Tenant 2"


exp_w$size.L1 <- 3072 - exp_w$size.L1
df_merged <-  rbind(zipf_w, exp_w)

#print(df_merged)




pdf(file="prop-partition.pdf",width=3,height=2.5)
ggplot(data=df_merged, aes(x=size.L1,y=latency,group=workload))+
 geom_point(aes(colour=workload,shape=workload))+
 geom_line(aes(colour=workload))+
 geom_text(aes(label="memory partition",x=1664,y=2.2,angle=90),fontface="italic",colour="dark green",size=3,vjust=-1.5)+
 geom_vline(xintercept=1664,size=0.3)+
 scale_y_continuous(trans = "log2",breaks=c(0.125,0.25,0.5,1,2,4,8,16))+
 scale_x_continuous(breaks=c(0,1,1.63,2,3)*1024,labels=c(0,1,1.63,2,3))+
 theme(legend.position="top",plot.margin=unit(c(0,0.1,0,0),"cm"),legend.title=element_blank(),legend.margin=unit(0,"cm"))+
 xlab("DRAM Partition for T1 (GB)")+
 ylab("Latency (ms)")
dev.off()

df_merged2 <-  merge(zipf_w, exp_w, by=c("size.L1"))

colnames(df_merged2) <- c("size.L1.zipf","size.L2i.zipf","mr.L1.zipf","mr.L2.zipf","size.L2e.zipf","cost.zipf","latency.zipf","size.L1.exp","size.L2i.exp","mr.L1.exp","mr.L2.exp","size.L2e.exp","cost.exp","latency.exp")

write.table(file="zipf-exp-power-partition.csv",df_merged2,sep="\t",row.names=FALSE)
