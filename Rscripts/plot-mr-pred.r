#!/usr/bin/env Rscript
library(ggplot2)
library(reshape2)
library(plyr)
library(gridExtra)
library(grid)

zipf.pred <- read.table(file.path("..","memcached-data","mrc-4Bkey-800Mreq-1.15alpha.csv"),sep="\t",col.names=c("size","mr"))
#zipf.pred$workload <- c("zipf_predicted")


zipf.measured <- read.table(file.path("..","memcached-data","mr-measure-4Bkeys-800Mreq-a1.15.csv"),sep="\t",col.names=c("size","mr"))
#zipf.measured[,"workload"] <- c("zipf_measured")
zipf.measured[,"workload"] <- c("Measured")


exp.pred <- read.table(file.path("..","memcached-data","mrc-4Bkey-800Mreq-exp0.000001.csv"), sep="\t", col.names=c("size","mr"))
#exp.measured[,"workload"] <- c("exp_predicted")

exp.measured <- read.table(file.path("..","memcached-data","mr-measure-800Mreq-exp0.000001.csv"), sep="\t", col.names=c("size","mr"))
exp.measured[,"workload"] <- c("exp_measured")


zipf.pred.approx = approx(x = zipf.pred$size, y = zipf.pred$mr, xout= zipf.measured$size)
zipf.pred.approx = data.frame(size=zipf.pred.approx$x, mr = zipf.pred.approx$y, workload=c("Predicted"))

exp.pred.approx = approx(x = exp.pred$size, y = exp.pred$mr, xout= exp.measured$size)
exp.pred.approx = data.frame(size=exp.pred.approx$x, mr = exp.pred.approx$y, workload=c("exp_predicted"))

write.table(zipf.pred.approx,file="zipf.predicted.csv",sep="\t")


#df = rbind(zipf.pred.approx, zipf.measured, exp.pred.approx, exp.measured)
df = rbind(zipf.pred.approx, zipf.measured)


df$latency = df$mr*10/100+(1-df$mr)*0.1/100
print(df)

pdf(file.path("results","rd-accuracy.pdf"),height=2.7,width=2.5)
ggplot(df, aes(x=size, y=mr,group=workload))+
    geom_point(aes(colour=workload,shape=workload))+
		theme(legend.position="top",legend.title=element_blank(),plot.margin=unit(c(0,0.4,0,0),"cm"), legend.margin=unit(0,"cm"))+
		scale_x_continuous(trans = "log2",breaks=c(64,256,1024,4096,16384))+
		labs(x="Cache Size (MB)", y="Miss Rate (%)")
		#theme(axis.title.x = element_text(size=7))
dev.off()

pdf(file.path("results","lat-accuracy.pdf"),height=4,width=6)
ggplot(df, aes(x=size, y=latency,group=workload))+
    geom_point(aes(colour=workload),shape=workload)+
		theme(legend.position="top",legend.title=element_blank())+
		labs(title="Latency Prediction on a Zipfian Workload", x="Memory Allocated (MB)", y="Latency(ms)")
dev.off()


df.measured <- df[df$workload == "exp_measured" | df$workload == "zipf_measured",]
pdf(file.path("results","mr-measured.pdf"),height=4,width=5)
ggplot(df.measured, aes(x=size,y=mr,group=workload))+
   geom_point(aes(shape=workload,colour=workload))+
		theme(legend.position="top",legend.title=element_blank())+
		labs(x="Memory (MB)", y="Miss Ratio")
dev.off()

df.cast.mr <- dcast(df, size~workload, value.var = c("mr"))
df.cast.lat <- dcast(df, size~workload, value.var = c("latency"))

print(df.cast.mr)

mape <- function(y, yhat)
	mean(abs((y-yhat)/y))

print(mape(df.cast.mr$zipf_measured,df.cast.mr$zipf_predicted))
print(mape(df.cast.lat$zipf_measured,zipf.cast.lat$zipf_predicted))

