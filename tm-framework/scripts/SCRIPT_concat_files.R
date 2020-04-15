#!/usr/bin/env Rscript

# ###########################################
# ### CONCATS FILES BY THE GIVEN ORDER ######
# ###########################################
# Usage: script.R <file1> ... <fileN>
# output: 
# > cat.txt
# ###########################################
# samples format:
#     "PAR1\tPAR2...\nPAR_N\t"
# ###########################################

arguments <- commandArgs(TRUE)
argc <- length(arguments)
cated <- NULL

for (i in 1:argc) {
	f <- arguments[i]
	csvs <- read.csv(f, sep='\t')

	if (is.null(cated)) {
		cated = csvs
	} else {
		if (nrow(csvs) < nrow(cated)) {
			pad = matrix(NA, nrow(cated) - nrow(csvs), ncol(csvs))
			colnames(pad) = colnames(csvs)
			csvs = rbind(csvs, pad)
		} else if (nrow(csvs) > nrow(cated)) {
			pad = matrix(NA, nrow(csvs) - nrow(cated), ncol(cated))
			colnames(pad) = colnames(cated)
			cated = rbind(cated, pad)
		}
	
		cated = cbind(cated, csvs)
	}
}

write.table(cated, "cat.txt", sep="\t", row.names=FALSE, col.names=TRUE)

