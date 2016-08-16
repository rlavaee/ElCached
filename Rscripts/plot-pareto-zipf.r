#!/usr/bin/env Rscript
library(ggplot2)
library(reshape2)
library(plyr)
library(gridExtra)

df <- read.table(file.path("zipf-pareto-power-partition.csv"),sep="\t")

ggplot(df,
