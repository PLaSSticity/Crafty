#!/usr/bin/env Rscript

# ###########################################
# ### COMPUTE AVERAGE OF SAMPLES AND ########
# ### CONFIDENCE INTERVAL AT 90% ############
# ###########################################
# Usage: script.R <sample1> <sample2> ...
# output: 
# > avg.txt # error columns append in the end
# ###########################################
# samples format:
#     "PAR1\tPAR2...\nPAR_N\t"
# ###########################################

arguments <- commandArgs(TRUE)
argc <- length(arguments)
files <- list()

for (i in 1:argc) {
	files[[i]] <- arguments[i]
}

csvs <- lapply(files, read.csv, sep='\t')
csvs <- lapply(csvs, function(x) round(x, digits=12))
means <- Reduce("+", lapply(csvs, function(x) replace(x, is.na(x), 0))) / Reduce("+", lapply(csvs, Negate(is.na)))
st.dev <- lapply( csvs, function(x) ( x - means )^2 )
st.dev <- sqrt( Reduce("+", lapply(st.dev, function(x) replace(x, is.na(x), 0))) / Reduce("+", lapply(st.dev, Negate(is.na))) )
error <- ( qnorm(0.950)*st.dev / sqrt(length( csvs ) )) # 90% conf. interval

avg_file <- "avg.txt"
avg <- cbind(means, error)

for (i in 1:length(avg)) {
	colnames(avg)[i] = paste("(", i, ")", colnames(avg)[i], sep="")
}

write.table(avg, avg_file, sep="\t", row.names=FALSE, col.names=TRUE)
