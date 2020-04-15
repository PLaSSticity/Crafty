#!/bin/bash

# ###########################################
# ### CUT FILE INTO CHUNCKS #################
# ###########################################
# Usage: script.sh <marker> <samples> ...
# > <sample>.c<i>
# where <i> is an index
# NOTE: perserves the header in all files
# ###########################################

if [[ $# -lt 2 ]] ; then
	echo "Usage: $0 <marker> <sample1> ... <sampleN>"
	exit
fi

MARKER=$1

for i in "$@"
do
	if [[ $i == $1 ]] ; then
		continue # marker
	fi
	
	#TODO: ...
	
done
