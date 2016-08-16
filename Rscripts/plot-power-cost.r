#!/usr/bin/env Rscript
library(ggplot2)
library(reshape2)
library(plyr)
library(gridExtra)

args <- commandArgs(TRUE)

workload <- read.table(args[1],sep="\t",col.names=c("size","mr"))
workload$mr <- workload$mr/100

t <- tail(workload,n=1)
workload <- rbind(workload, c(t[1,1]*10,t[1,2]))
workload <- rbind(workload, c(0,1))


workload.approx = approx(workload$size, workload$mr, xout = c(seq(0,30000,10)))


workload.approx.power = approx(workload$size, workload$mr, xout=workload.approx$x*5)


df = data.frame(size.L1=workload.approx$x, size.L2i=workload.approx.power$x, mr.L1 = workload.approx$y, mr.L2 = workload.approx.power$y)


df$size.L2e = df$size.L2i - df$size.L1

df$cost = df$size.L1*10/1024 + df$size.L2e*0.68/1024
df$latency = df$mr.L2*10+(df$mr.L1-df$mr.L2)*0.2 + (1-df$mr.L1)*0.1
print(colnames(df))

df$latency = ceiling(df$latency*100)/100




df.mincost <- do.call(rbind,lapply(split(df,df$latency),function(chunk) chunk[which.min(chunk$cost),]))

print(df.mincost)

write.table(file="workload-power-cost.csv",df.mincost,sep="\t",row.names=FALSE)

pdf(file="power-cost.pdf",width=4,height=4)
ggplot(data=df.mincost, aes(x=latency, y=cost))+
  geom_point()
dev.off()
