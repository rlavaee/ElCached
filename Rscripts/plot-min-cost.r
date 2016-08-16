#!/usr/bin/env Rscript
library(ggplot2)
library(reshape2)
library(plyr)
library(gridExtra)

args <- commandArgs(TRUE)

workload <- read.table(args[1],sep="\t",col.names=c("size","mr"))
workload$mr <- workload$mr/100

t <- tail(workload,n=1)
workload <- rbind(workload, c(t[1,1]*4,t[1,2]))
workload <- rbind(workload, c(0,1))

workload.approx = approx(workload$size, workload$mr, xout = c(seq(0,30000,10)))

df = data.frame(size.L1=workload.approx$x, mr.L1=workload.approx$y)


latencies <- data.frame(latency=seq(0,2,0.01))

df <- Reduce(function(...) merge(..., all=T, by=NULL), list(df,latencies))

df$mr.L2 <- (df$latency - df$mr.L1*0.1 - 0.1) / 9.8

workload.approx.L2i = approx(workload$mr, workload$size, xout=df$mr.L2)

df$size.L2i <- workload.approx.L2i$y

df$size.L2e <- df$size.L2i - df$size.L1

df$cost = df$size.L1*10/1024 + df$size.L2e*0.68/1024

#print(df)




df <- df[complete.cases(df),]
df <- df[(df$size.L2i >= df$size.L1) & (df$mr.L2 <= 100) & (df$mr.L2 >= 0),]


df.mincost <- do.call(rbind,lapply(split(df,df$latency),function(chunk) chunk[which.min(chunk$cost),]))

df.mincost <- df.mincost[,!(names(df.mincost) %in% c("size.L2i"))]

#print(df.mincost)

write.table(file="workload-min-cost.csv",df.mincost,sep="\t",row.names=FALSE)

pdf(file="min-cost.pdf",width=4,height=4)
ggplot(data=df.mincost, aes(x=latency, y=cost))+
  geom_point()
dev.off()
