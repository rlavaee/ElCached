#!/usr/bin/env Rscript
library(ggplot2)
df <- data.frame(x1 = c(1.3,2.62,4,10), y1 = c(2,10,21.0,23), y2 = c(20,10,15.0,8))

pdf(file="example.pdf",width=4,height=4)
  ggplot(df)+
  geom_line(aes(x=x1,y=y1),colour='blue')+
	geom_line(aes(x=x1,y=y2),colour='green')+
	geom_ribbon(aes(x=x1,ymin=y2,ymax=pmax(y1,y2)),fill='gray')
dev.off()
