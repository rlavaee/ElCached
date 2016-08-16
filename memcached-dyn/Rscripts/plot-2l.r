#!/usr/bin/env Rscript
library(ggplot2)
library(reshape2)
library(plyr)
library(gridExtra)

zipf <- read.table(file.path("..","mrc-4Bkey-50Mreq-1.15alpha.csv"),sep="\t",col.names=c("size","mr"))
exp <- read.table(file.path("..","mrc-4Bkey-50Mreq-exp0.00001.csv"),sep="\t",col.names=c("size","mr"))
zipf[,"workload"] <- c("zipf-1.15-4Bkey-50Mreq")
exp[,"workload"] <- c("exp-0.00001-4Bkey-50Mreq")

zipf.approx = approx(zipf$size, zipf$mr, n=50)
exp.approx = approx(exp$size, exp$mr, n=50)
zipf = data.frame(size=zipf.approx$x, mr=zipf.approx$y)
colnames(zipf)=c("size.zipf","mr.zipf")
exp = data.frame(size=exp.approx$x, mr=exp.approx$y)
colnames(exp) = c("size.exp","mr.exp")
#df <- rbind(zipf,exp)


df.join <- Reduce(function(...) merge(..., all=T, by=NULL), list(zipf,zipf,exp,exp))
print(colnames(df.join))

df.join <- df.join[(df.join$size.zipf.x <= df.join$size.zipf.y) & (df.join$size.exp.x < df.join$size.exp.y),]

print(df.join)

df.join$mr = df.join$mr.x + df.join$mr.y
df.join$size = df.join$size.x + df.join$size.y

df.join$latency = df.join$mr*10/100+(1-df.join$mr)*0.1/100

df.join$cost = df.join$size*10/1024

df.join = df.join[, !(names(df.join) %in% c("workload.x","workload.y","mr.x","mr.y"))]
#print(df.join)

#df.table <- acast(df.join, size.x~size.y, value.var="latency.total")

#print(df.table)

#ggplot(data=df.join, aes(x=size.x,y=size.y,color=latency.total)) + 
#		geom_point()+
#		scale_colour_gradient(low="black", high="green")

colnames(df.join)


costs <- seq(1,20,1)

find_min_lat <- function(c) {
	x = df.join[df.join$cost < c,]
  x[which.min(x$latency),]
}

min_lats <- ldply(costs, find_min_lat)
min_lats <- round(min_lats,2)
min_lats <- min_lats[,c(6:5,1:4)]

min_lats.m <- melt(min_lats, id=c("cost","size","mr"), value.name="allocation")

min_lats.m$plot_group <- sapply(min_lats.m$variable, function(x) {if(x=="latency") return("Latency (ms)") else return("Allocation (MB)")})

print(min_lats.m)

pdf(file.path("results","min-lats-plot.pdf"),height=8,width=10)
ggplot(min_lats.m, aes(x=cost, y=allocation,group=variable))+
    facet_grid(plot_group~.,scales="free")+
    geom_point(aes(shape=variable, colour=variable))+
		scale_shape_discrete(name="workload",breaks=c("size.x","size.y","latency"),labels=c("zipfian","exponential","mixed"))+
		scale_colour_discrete(name="workload",breaks=c("size.x","size.y","latency"),labels=c("zipfian","exponential","mixed"))+
		labs(x = "Cost ($)",y="", title="Optimal Latency Configuration For Each Cost Limit")
dev.off()


colnames(min_lats) <- c("cost limit($)","min latency(ms)","allocation.x(MB)","allocation.y(MB)","miss ratio","total allocation")
#print(min_lats)

write.table(file=file.path("results","min-lats.csv"),min_lats,sep="\t",row.names=FALSE)
pdf(file.path("results","min-lats.pdf"),height=12,width=10)
grid.table(min_lats,rows=NULL)
dev.off()




lats <- seq(0,6,0.1)
find_min_cost <- function(l) {
  x = df.join[df.join$latency < l,]
  x[which.min(x$cost),]
}
min_costs <- ldply(lats, find_min_cost)
min_costs <- round(min_costs,2)
min_costs <- min_costs[,c(5:6,1:4)]

min_costs.m <- melt(min_costs, id=c("latency","size","mr"), value.name="allocation")

min_costs.m$plot_group <- sapply(min_costs.m$variable, function(x) {if(x=="cost") return("Cost ($)") else return("Allocation (MB)")})


pdf(file.path("results","min-costs-plot.pdf"),height=8,width=10)
ggplot(min_costs.m, aes(x=latency, y=allocation,group=variable))+
    facet_grid(plot_group~.,scales="free")+
    geom_point(aes(shape=variable, colour=variable))+
		scale_shape_discrete(name="workload",breaks=c("size.x","size.y","cost"),labels=c("zipfian","exponential","mixed"))+
		scale_colour_discrete(name="workload",breaks=c("size.x","size.y","cost"),labels=c("zipfian","exponential","mixed"))+
		labs(x = "Latency (ms)",y="", title="Optimal Cost Configuration For Each Latency Limit")
dev.off()


colnames(min_costs) <- c("latency limit(ms)","min cost($)","allocation.x(MB)","allocation.y(MB)","miss ratio","total allocation(MB)")
#print(min_costs)

write.table(file=file.path("results","min-costs.csv"),min_costs,sep="\t",row.names=FALSE)
pdf(file.path("results","min-costs.pdf"),height=12,width=10)
grid.table(min_costs,rows=NULL)
dev.off()



#library(scatterplot3d)


#with(df.join, {
#		 scatterplot3d(size.x,size.y,latency,
#									 color="blue", pch=19, type="h")})

