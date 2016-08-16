#!/usr/bin/env Rscript
library(ggplot2)
library(reshape2)
library(plyr)
library(gridExtra)
library(grid)

df.min <- read.table(file="zipf-min-cost.csv",sep="\t",header=TRUE)
df.min$method <- "Elastic"
df.min$workload <- "Zipfian"

df.power <- read.table(file="zipf-power-cost.csv",sep="\t",header=TRUE)
df.power$method <- "Prop."
df.power$workload <- "Zipfian"

df.power <- df.power[,!(names(df.power) %in% c("size.L2i"))]

df.rel = merge(df.min,df.power,by=c("latency","workload"))
df.rel$ncost = df.rel$cost.x / df.rel$cost.y
df.rel$method = "Elastic/Prop."
df.rel$plott = "Relative Cost"
df.rel$workload = "Zipfian"

df_merged1 = rbind(df.min,df.power)
df_merged1$plott = "Normalized Cost"

lats <- intersect(df.power$latency,df.min$latency)
lats <- c(0.67,0.68,0.69,0.70,0.71,0.72,0.74,0.76,0.78,0.80,0.84,0.88,0.92,0.96,1.00)



print(df.power[df.power$latency == 1,]$cost)
df_merged1$ncost = df_merged1$cost / df.power[df.power$latency == 1,]$cost
df_merged1 = df_merged1[,names(df_merged1) %in% c("workload","method","latency","ncost","plott")]
df.rel = df.rel[,names(df.rel) %in% c("workload","method","latency","ncost","plott")]



df_merged1 = rbind(df.rel,df_merged1)
df_merged1 = df_merged1[(df_merged1$latency<= 1) & (df_merged1$latency %in% lats),]

#print(df_merged1)

df.min <- read.table(file="exp-min-cost.csv",sep="\t",header=TRUE)
df.min$method <- "Elastic"
df.min$workload <- "Exponential"

df.power <- read.table(file="exp-power-cost.csv",sep="\t",header=TRUE)
df.power$method <- "Prop."
df.power$workload <- "Exponential"

df.power <- df.power[,!(names(df.power) %in% c("size.L2i"))]



df.rel = merge(df.min,df.power,by=c("latency","workload"))
df.rel$ncost = df.rel$cost.x / df.rel$cost.y
df.rel$method = "Elastic/Prop."
df.rel$plott = "Relative Cost"

df_merged2 = rbind(df.min,df.power)
df_merged2$plott = "Normalized Cost"


lats <- intersect(df.power$latency,df.min$latency)
lats <- c(0.20,0.21,0.22,0.23,0.24,0.25,0.28,0.31,0.36,0.41,0.48,0.56,0.65,0.77,0.85,0.93,1.00)

print(df.power[df.power$latency == 1,]$cost)
df_merged2$ncost = df_merged2$cost / df.power[df.power$latency == 1,]$cost

df_merged2 = df_merged2[,names(df_merged2) %in% c("workload","method","latency","ncost","plott")]
df.rel = df.rel[,names(df.rel) %in% c("workload","method","latency","ncost","plott")]

df_merged2 <- rbind(df_merged2,df.rel)
df_merged2 = df_merged2[(df_merged2$latency<= 0.6) & (df_merged2$latency %in% lats),]
#print(df_merged2)


df_merged <- rbind(df_merged2,df_merged1)

df_merged$method <- factor(df_merged$method, levels=c("Prop.","Elastic","Elastic/Prop."))

df_merged$workload <- factor(df_merged$workload, levels=c("Zipfian","Exponential"))

#print(df_merged[df_merged$latency == 0.7,])
#print(df_merged)



pdf(file="prop-vs-elastic.pdf",width=4,height=3)
ggplot(data=df_merged, aes(x=latency,y=ncost,group=method))+
 facet_grid(plott~workload,scales="free")+
 geom_point(aes(colour=method,shape=method))+
 geom_line(aes(colour=method))+
 theme(legend.position="top",plot.margin=unit(c(0,0.3,0,0),"cm"),legend.title=element_blank(),axis.title.y=element_blank(),panel.margin = unit(0.3,"cm"),strip.text.y=element_text(size=8),legend.margin=unit(0,"cm"))+
 xlab("Latency (ms)")
dev.off()
