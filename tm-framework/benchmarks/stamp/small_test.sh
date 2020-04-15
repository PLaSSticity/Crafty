#!/bin/bash

rm -f small_test.txt 

THREADS=14
UPDATE=50
NB_OPERATIONS=999999
NB_INIT_INS=99999
RANGE_SIZE=99999999

run_intruder() {

	
	if [[ $? -ne 0 ]] ; then
		# try one more
		for t in `seq 8`
		do
			for t in `seq 8`
			do
				MAKEFILE_ARGS="SOLUTION=3 DO_CHECKPOINT=1" ./build-stamp.sh htm-sgl-nvm intruder_HTM_t"$t"_l"$l".txt
			done
		done
		timeout 10s ./intruder/intruder
	fi
	
}

BENCH=./intruder/intruder

-a10 -l128 -n262144 -s1 -r1 -t

~/projects/cp_nb_projects.sh >/dev/null

echo "\tTIME\tNB_TXS\tNB_HTM_COMMITS\tNB_SGL_COMMITS\tABORTS" \
		"\tCAPACITY\tCONFLICTS\n" >> small_test.txt

echo "NO_CHECKPOINT\n" >> small_test.txt  
./build-nvmhtm.sh normal_new_val small_test.txt  
for n in $THREADS 
do
	$BENCH -a10 -l128 -n10000 -s1 -r1 -t$THREADS
done

echo "PERIODIC_CHECKPOINT\n" >> small_test.txt   
./build-nvmhtm.sh normal_checkpoint_p small_test.txt 10000
for n in $THREADS 
do
	$BENCH -a10 -l128 -n10000 -s1 -r1 -t$THREADS 
done

echo "REACTIVE_CHECKPOINT\n" >> small_test.txt  
./build-nvmhtm.sh normal_checkpoint_r small_test.txt 10000
for n in $THREADS 
do
	$BENCH -a10 -l128 -n10000 -s1 -r1 -t$THREADS  
done

# ./build-nvmhtm.sh normal_no_val small_test.txt 
# for n in 14 
# do
	# ./hashmap/hashmap -n$n -u$UPDATE -d999999 -i99999 -r99999999
# done

echo "AVNI\n" >> small_test.txt  
./build-nvmhtm.sh avni small_test.txt
for n in $THREADS
do
   $BENCH -a10 -l128 -n10000 -s1 -r1 -t$THREADS 
done

echo "HTM_ONLY\n" >> small_test.txt  
./build-nvmhtm.sh htm_only small_test.txt
for n in $THREADS 
do
   $BENCH -a10 -l128 -n10000 -s1 -r1 -t$THREADS 
done
