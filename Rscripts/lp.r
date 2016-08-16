#!/usr/bin/env Rscript 
library(lpSolveAPI)
lp <- make.lp(0,2)
add.constraint(lp, c(1, 8/3), "<=", 4)
add.constraint(lp, c(1, 1),   "<=", 2)
add.constraint(lp, c(2, 0),   "<=", 3)
set.objfn(lp, c(1,2))
pdf(file="lp.pdf",width=5,height=5)
plot(lp)
dev.off()
				         
