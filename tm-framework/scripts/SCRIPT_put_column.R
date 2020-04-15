#!/usr/bin/env Rscript

# ###########################################
# ### COMPUTE NEW COLUMNS IN SAMPLE #########
# ###########################################
# Usage: script.R <formulas> <samples>
# matrix in formulas are accessed using:
# > a[row,col]
# assume R syntax 
# output: 
# > <sample>.cols
# ###########################################
# samples format:
#     "PAR1\tPAR2...\nPAR_N\t"
# ###########################################

arguments <- commandArgs(TRUE)
argc <- length(arguments)
formulas <- arguments[1]

for (i in 2:argc) {
	f <- arguments[i]
	new_f <- paste(f, ".cols", sep="")
	csvs <- read.csv(f, sep='\t')
	
	print(paste("proc file", f))
	
	a <- csvs # debug
	
	eval(parse(text=formulas))
	
	write.table(a, new_f, sep="\t", row.names=FALSE, col.names=TRUE)
}
