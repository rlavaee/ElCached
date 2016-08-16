#!/usr/bin/env Rscript
library(ggplot2)
library(reshape2)
library(plyr)
library(gridExtra)

args <- commandArgs(TRUE)

workload <- read.table(args[1],sep="\t",col.names=c("size","mr"))

workload.approx = approx(workload$size, workload$mr, xout = c(seq(0,180,20),seq(200,2000,40)))

df = data.frame(size.L1=workload.approx$x, mr.L1=workload.approx$y)


costs <- data.frame(cost=seq(0,20,0.1))

df <- Reduce(function(...) merge(..., all=T, by=NULL), list(df,costs))

df$size.L2i <- ((df$cost - (df$size.L1 * 10/1024))/ 0.68 * 1024) + df$size.L1


df <- df[complete.cases(df),]
df <- df[df$size.L2i >= df$size.L1,]


workload.approx.L2i = approx(workload$size, workload$mr, xout = df$size.L2i)

df$mr.L2i <- workload.approx.L2i$y

df$size.L2e = df$size.L2i - df$size.L1

df$latency = df$mr.L2i*10/100+(df$mr.L1-df$mr.L2i)*0.2/100 + (1-df$mr.L1)*0.1/100


#print(split(df,df$cost))

df.minlat <- do.call(rbind,lapply(split(df,df$cost),function(chunk) chunk[which.min(chunk$latency),]))

df.minlat <- df.minlat[,!(names(df.minlat) %in% c("size.L2i"))]

#print(df.minlat)

write.table(file="workload-min-latency.csv",df.minlat,sep="\t",row.names=FALSE)

pdf(file="min-lat.pdf",width=4,height=4)
ggplot(data=df.minlat, aes(x=cost, y=latency))+
  geom_point()
dev.off()
