#!/bin/sh


threads=$1

test[1]="./genome/genome -g16384 -s64 -n16777216 -r1 -t" 
test[2]="./intruder/intruder -a10 -l128 -n262144 -s1 -r1 -t" 
test[3]="./kmeans/kmeans -m15 -n15 -t0.00001 -i kmeans/inputs/random-n65536-d32-c16.txt -r1 -p" 
test[4]="./kmeans/kmeans -m40 -n40 -t0.00001 -i kmeans/inputs/random-n65536-d32-c16.txt -r1 -p" 
test[5]="./labyrinth/labyrinth  -i labyrinth/inputs/random-x512-y512-z7-n512.txt -t" 
test[6]="./ssca2/ssca2 -s20 -i1.0 -u1.0 -l3 -p3 -r1 -t" 
test[7]="./vacation/vacation -n4 -q60 -u90 -r1048576 -t4194304 -a1 -c" 
test[8]="./vacation/vacation  -n2 -q90 -u98 -r1048576 -t4194304 -a1 -c" 
test[9]="./yada/yada -a15 -i yada/inputs/ttimeu1000000.2 -r1 -t"

for t in 1 2 3 4 6 7 8 9
do

echo "Executing: ${test[$t]} ${threads}"
${test[$t]} $threads > $2/$t

done


