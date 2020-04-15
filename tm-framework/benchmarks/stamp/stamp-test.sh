#!/bin/sh



test[1]="./genome/genome -g256 -s16 -n16384 -r100 -t" 
test[2]="./intruder/intruder -a10 -l4 -n2048 -s1 -r100 -t" 
test[3]="./kmeans/kmeans -m15 -n15 -t0.05 -i kmeans/inputs/random-n2048-d16-c16.txt -r100 -p" 
test[4]="./kmeans/kmeans -m40 -n40 -t0.05 -i kmeans/inputs/random-n2048-d16-c16.txt -r100 -p" 
test[5]="./labyrinth/labyrinth -i labyrinth/inputs/random-x32-y32-z3-n96.txt -t" 
test[6]="./ssca2/ssca2 -s13 -i1.0 -u1.0 -l3 -p3 -r100 -t" 
test[7]="./vacation/vacation -n4 -q60 -u90 -r16384 -t4096 -a100 -c" 
test[8]="./vacation/vacation  -n2 -q90 -u98 -r16384 -t4096 -a100 -c" 
test[9]="./yada/yada  -a20 -i yada/inputs/633.2 -r100 -t"


for b in 1 2 3 4 6 7 8 9
do
#for t in 1 4 8 16 32 64 80
#do
	echo "Executing: ${test[$b]} $1"
	${test[$b]} $1 > $2/$b
	rc=$?
	if [[ $rc != 0 ]] ; then
        	exit 1
    	fi
#done
done


