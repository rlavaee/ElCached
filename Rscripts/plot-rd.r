#!/usr/bin/env Rscript
library(ggplot2)
df <- data.frame(reuse.dist=c(1,2,3,4,5),freq=c(0,0, 1,2,4))
df1 <- data.frame(cache.size=c(0,1,2,3,4,5),mr=c(1,1,1,6/7,4/7,4/7))
pdf(file="hist.pdf",width=2,height=2)
ggplot(df)+
geom_bar(aes(x=reuse.dist,y=freq),stat="identity")+
xlab("reuse distance")+
ylab("frequency")+
scale_x_discrete(breaks=c(1,2,3,4,5),labels=c(1,2,3,4,expression(infinity)))
dev.off()

pdf(file="mr.pdf",width=2,height=2)
ggplot(df1)+
geom_line(aes(x=cache.size,y=mr))+
xlab("cache size")+
ylab("miss ratio")+
ylim(c(0,1))
dev.off()
