#!/usr/bin/env Rscript
library(ggplot2)
library(reshape2)
library(plyr)
library(gridExtra)

zipf <- read.table(file.path("..","memcached-data","mrc-4Bkey-800Mreq-1.12alpha.csv"),sep="\t",col.names=c("size","mr"))
#exp <- read.table(file.path("..","memcached-data","mrc-4Bkey-800Mreq-exp0.000001.csv"),sep="\t",col.names=c("size","mr"))
exp <- read.table(file.path("..","memcached-data","mrc-800Mreq-pareto-scale20-shape5.csv"),sep="\t",col.names=c("size","mr"))
zipf[,"workload"] <- c("zipf-1.15-4Bkey-800Mreq")
exp[,"workload"] <- c("exp-0.00001-4Bkey-800Mreq")

zipf <- rbind(zipf, c(0,100))
exp <- rbind(exp, c(0,100))

zipf.approx = approx(zipf$size, zipf$mr, xout = c(seq(0,180,20),seq(200,2000,40)))
#print(zipf.approx)
exp.approx = approx(exp$size, exp$mr, xout =  c(seq(0,180,20),seq(200,2000,40)))



zipf.extra.L1 = approx(x = zipf$size, y = zipf$mr, xout = c(200,100))
zipf.extra.L2 = approx(x = zipf$size, y = zipf$mr, xout = c(1000,500))

exp.extra.L1 = approx(x = exp$size, y = exp$mr, xout = c(200,300))
exp.extra.L2 = approx(x = exp$size, y = exp$mr, xout = c(1000,1500))

df.extra = data.frame(size.zipf.L1 = zipf.extra.L1$x, size.zipf.L2i = zipf.extra.L2$x, size.exp.L1 = exp.extra.L1$x, size.exp.L2i = exp.extra.L2$x, mr.zipf.L1 = zipf.extra.L1$y, mr.zipf.L2i = zipf.extra.L2$y, mr.exp.L1 = exp.extra.L1$y, mr.exp.L2i = exp.extra.L2$y, points = "manual")

#print(df.extra)

zipf = data.frame(size=zipf.approx$x, mr=zipf.approx$y)
colnames(zipf)=c("size.zipf","mr.zipf")
exp = data.frame(size=exp.approx$x, mr=exp.approx$y)
colnames(exp) = c("size.exp","mr.exp")


df.join <- Reduce(function(...) merge(..., all=T, by=NULL), list(zipf,zipf))

names(df.join)[names(df.join) == "size.zipf.x"] = "size.zipf.L1"
names(df.join)[names(df.join) == "size.zipf.y"] = "size.zipf.L2i"
names(df.join)[names(df.join) == "mr.zipf.x"] = "mr.zipf.L1"
names(df.join)[names(df.join) == "mr.zipf.y"] = "mr.zipf.L2i"

df.join2 <- Reduce(function(...) merge(..., all=T, by=NULL), list(zipf,exp))

print(df.join2)

names(df.join2)[names(df.join2) == "size.zipf"] = "size.zipf.L1"
names(df.join2)[names(df.join2) == "size.exp"] = "size.exp.L1"
names(df.join2)[names(df.join2) == "mr.zipf"] = "mr.zipf.L1"
names(df.join2)[names(df.join2) == "mr.exp"] = "mr.exp.L1"

df.join <- df.join[(df.join$size.zipf.L1 <= df.join$size.zipf.L2i),]

exp.L1 = 400 - df.join$size.zipf.L1
exp.L2i = 2000 - df.join$size.zipf.L2i

exp.approx.L1 = approx(exp$size, exp$mr, xout = exp.L1)
exp.approx.L2i = approx(exp$size, exp$mr, xout = exp.L2i)

df.join$size.exp.L1 = exp.approx.L1$x
df.join$size.exp.L2i = exp.approx.L2i$x
df.join$mr.exp.L1 = exp.approx.L1$y
df.join$mr.exp.L2i = exp.approx.L2i$y


df.join$points <- "partitions"

zipf.L2i = ((2.48 - (df.join2$size.zipf.L1 * 10/1024))/ 0.68 * 1024) + df.join2$size.zipf.L1
exp.L2i = ((2.48 - (df.join2$size.exp.L1 * 10/1024)) / 0.68 * 1024) + df.join2$size.exp.L1



zipf.approx.L2i = approx(zipf$size, zipf$mr, xout = zipf.L2i)
exp.approx.L2i = approx(exp$size, exp$mr, xout = exp.L2i)

df.join2$size.zipf.L2i = zipf.approx.L2i$x
df.join2$mr.zipf.L2i = zipf.approx.L2i$y
df.join2$size.exp.L2i = exp.approx.L2i$x
df.join2$mr.exp.L2i = exp.approx.L2i$y

df.join2$points <- "fixed-cost"

#print(df.join2)

df <- rbind(df.join,df.join2,df.extra)


df$mr.L1 = df$mr.zipf.L1 + df$mr.exp.L1
df$mr.L2 = df$mr.zipf.L2i + df$mr.exp.L2i

df$size.zipf.L2e <- df$size.zipf.L2i - df$size.zipf.L1
df$size.exp.L2e <- df$size.exp.L2i - df$size.exp.L1

df$size.L1 = df$size.zipf.L1 + df$size.exp.L1
df$size.L2 = df$size.zipf.L2e + df$size.exp.L2e

#print(nrow(df))


df$latency = df$mr.L2*10/100+(df$mr.L1-df$mr.L2)*0.2/100 + (1-df$mr.L1)*0.1/100
df$zipf.latency = df$mr.zipf.L2i*10/100+(df$mr.zipf.L1-df$mr.zipf.L2i)*0.2/100 + (1-df$mr.zipf.L1)*0.1/100
df$exp.latency = df$mr.exp.L2i*10/100+(df$mr.exp.L1-df$mr.exp.L2i)*0.2/100 + (1-df$mr.exp.L1)*0.1/100

df$cost = df$size.L1*10/1024 + df$size.L2*0.68/1024

df$zipf.cost = df$size.zipf.L1*10/1024 + df$size.zipf.L2e*0.68/1024
df$exp.cost = df$size.exp.L1*10/1024 + df$size.exp.L2e*0.68/1024

find_hull <- function(df){
#	print(chull(df$zipf.latency, df$exp.latency))
	return(df[chull(df$zipf.latency, df$exp.latency),])
}




#df <- df[(df$points != "partitions") | ((df$zipf.cost < (1.10*3/2)) & (df$exp.cost < (1.10*3/2))),]

#df <- df[(df$points == "extra") | ((df$zipf.latency <= 2) & (df$exp.latency <= 1)),]

#print(nrow(df))

df = df[, (names(df) %in% c("workload","cost","zipf.cost","exp.cost", "size.zipf.L1","size.exp.L1","size.zipf.L2i","size.exp.L2i","size.zipf.L2e","size.exp.L2e","zipf.latency","exp.latency","latency","cost","points"))]

df <- df[complete.cases(df),]
df <- df[(df$size.zipf.L2e >= 0) & (df$size.exp.L2e >= 0) & (df$size.zipf.L1 >= 0) & (df$size.exp.L1 >= 0),]
#print(colnames(df))

#df[,-5] <- round(df[,-5],2)

df.p = df[df$points=="partitions" & df$zipf.latency < 1 & df$exp.latency < 1 & df$zipf.cost < 2.8 & df$exp.cost < 2.8,]
labels1 <- df.p[which(df.p$zipf.latency == min(df.p$zipf.latency)),][1,]

labels2 <- df[df$points=="manual",]

df.fc = df[df$points=="fixed-cost" & df$zipf.latency < 1 & df$exp.latency < 1,]
labels3 = df.fc[which(df.fc$exp.latency == min(df.fc$exp.latency)),][1,]

labels = rbind(labels1,labels2,labels3)
hjusts <- c(1.2,1.2,-0.2,0.8)
vjusts <- c(1.5,1.5,-0.5,1.2)

print(labels)


#print(df)

#hulls <- ddply(df, .(points),find_hull)

#print(hulls)

#print(df)


pdf(file.path("results","all-conf-2l.pdf"),height=8,width=10)
ggplot(df, aes(x=zipf.latency, y=exp.latency))+
	geom_point(aes(colour=points,shape=points),size=3)+
	geom_text(data=labels,aes(label=paste("zipf: (",round(size.zipf.L1),",",round(size.zipf.L2e),"):",round(zipf.cost,2),"$\nexp: (", round(size.exp.L1),",",round(size.exp.L2e),") :",round(exp.cost,2),"$")),vjust=vjusts,hjust=hjusts,size=6)+
	#geom_text(aes(label=paste("(",round(size.zipf.L1),",",round(size.zipf.L2e),")\n(", round(size.exp.L1),",",round(size.exp.L2e),")")),size=0.5)+
	geom_point(data=labels,aes(shape=points),size=3)+
	geom_line(aes(x=1),size=0.2)+
	geom_line(aes(y=1),size=0.2)+
	#geom_text(aes(label=paste("(",size.zipf.L1,",",size.zipf.L2e,")")),vjust="inward",hjust="inward",angle=45,size=0.1)+
  scale_x_continuous(limits=c(0.5,1.5))+
	scale_y_continuous(limits=c(0,1.8))+
  xlab("Latency of the Zipfian Workload (ms)")+ ylab("Latency of the Exponential Workload (ms)")+
	theme(axis.text.x = element_text(size=14), axis.title.x = element_text(size=18),
				axis.text.y = element_text(size=14), axis.title.y = element_text(size=18),
				legend.title = element_blank(), legend.text = element_text(size=16))
	#coord_fixed(ratio = 1)+
	#ylim(0,4)+xlim(0,4)
dev.off()


costs <- c(1.3,1.8,2.5,3,seq(4,200,5))

find_min_lat <- function(c) {
	x = df[df$cost < c,]
  x[which.min(x$latency),]
}


min_lats <- ldply(costs, find_min_lat)

min_lats <- min_lats[,c(7:6,1:5)]
min_lats[,-7] <- round(min_lats[,-7],2)

min_lats.m <- melt(min_lats, id=c("cost","points"), value.name="allocation")

min_lats.m$plot_group <- sapply(min_lats.m$variable, function(x) {if(x=="latency") return("Latency (ms)") else return("Allocation (MB)")})


pdf(file.path("results","min-lats-plot-2l.pdf"),height=8,width=30)
ggplot(min_lats.m, aes(x=cost, y=allocation,group=variable))+
    #geom_text(aes(label=get_label(allocation,plot_group)), vjust=0)+
    facet_grid(plot_group~.,scales="free")+
    geom_bar(stat="identity",width=0.3, position=position_dodge(width=0.4),aes(fill=variable))+
		theme(axis.text.x = element_text(size=14, angle=45), axis.title.x = element_text(size=18), title = element_text(size=20), axis.text.y=element_text(size=14) )+
		theme(legend.title = element_blank(), legend.text = element_text(size=16))+
		#scale_shape_discrete(name="workload",breaks=c("size.zipf.L1","size.zipf.L2i","size.exp.L1","size.exp.L2i","latency"),labels=c("Zipf-L1","Zipf-L2","Exp-L1","Exp-L2","mixed"))+
		scale_fill_discrete(name="workload",breaks=c("size.zipf.L1","size.zipf.L2i","size.exp.L1","size.exp.L2i","latency"),labels=c("Zipf-L1","Zipf-L2","Exp-L1","Exp-L2","mixed"))+
		scale_x_continuous(breaks=min_lats.m$cost,labels=min_lats.m$cost)+
		labs(x = "Cost ($)",y="", title="Optimal Latency Configuration For Each Cost Limit")
dev.off()


colnames(min_lats) <- c("cost limit($)","min latency(ms)","allocation.x(MB)","allocation.y(MB)","miss ratio","total allocation")

write.table(file=file.path("results","min-lats-2l.csv"),min_lats,sep="\t",row.names=FALSE)
pdf(file.path("results","min-lats-2l.pdf"),height=12,width=10)
grid.table(min_lats,rows=NULL)
dev.off()




lats <- seq(0,6,0.1)
find_min_cost <- function(l) {
  x = df[df$latency < l,]
  x[which.min(x$cost),]
}
min_costs <- ldply(lats, find_min_cost)
min_costs <- round(min_costs,2)
min_costs <- min_costs[,c(5:6,1:4)]

min_costs.m <- melt(min_costs, id=c("latency","size","mr"), value.name="allocation")

min_costs.m$plot_group <- sapply(min_costs.m$variable, function(x) {if(x=="cost") return("Cost ($)") else return("Allocation (MB)")})


pdf(file.path("results","min-costs-plot-2l.pdf"),height=6,width=8)
ggplot(min_costs.m, aes(x=latency, y=allocation,group=variable))+
    facet_grid(plot_group~.,scales="free")+
    geom_point(aes(shape=variable, colour=variable))+
		scale_shape_discrete(name="workload",breaks=c("size.x","size.y","cost"),labels=c("zipfian","exponential","mixed"))+
		scale_colour_discrete(name="workload",breaks=c("size.x","size.y","cost"),labels=c("zipfian","exponential","mixed"))+
		labs(x = "Latency (ms)",y="", title="Optimal Cost Configuration For Each Latency Limit")
dev.off()


colnames(min_costs) <- c("latency limit(ms)","min cost($)","allocation.x(MB)","allocation.y(MB)","miss ratio","total allocation(MB)")

write.table(file=file.path("results","min-costs-2l.csv"),min_costs,sep="\t",row.names=FALSE)
pdf(file.path("results","min-costs-2l.pdf"),height=12,width=10)
grid.table(min_costs,rows=NULL)
dev.off()



#library(scatterplot3d)


#with(df.join, {
#		 scatterplot3d(size.x,size.y,latency,
#									 color="blue", pch=19, type="h")})

