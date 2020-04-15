#!/bin/bash

SAMPLES=10
NB_TRANSFERS=5000000
BANK_BUDGET=100000

LOG_SIZE1=1000
LOG_SIZE2=10000
LOG_SIZE3=100000
LOG_SIZE4=1000000

# 10us
PERIOD1=10
PERIOD2=100

# 1ms
PERIOD3=1000
PERIOD4=10000
PERIOD5=100000

THRESHOLD2=0.10
THRESHOLD3=0.50
THRESHOLD4=0.90

FILTER1=0.01
FILTER2=0.05
FILTER3=0.10

BUILD_SCRIPT=./build.sh

NB_ACCOUNTS=32768

### Call with 1-TX_SIZE 2-UPDATE_RATE 3-THREADS 4-NB_ACCOUNTS 5-NB_TRANSFERS
function run_bench {
	echo -ne "0\t0\t$4\t$2\t$1\n" >> parameters.txt
	ipcrm -M 0x00054321
	timeout 3m ./bank/bank -a $4 -d $5 -b $BANK_BUDGET -u $2 -n $3 -s $1 
}

function bench_1 {
	for i in `seq $SAMPLES` 
	do
		rm -f parameters.txt stats_file stats_file.aux_thr
		for j in 4
		do
			for u in 50
			do
				echo -ne "#SEQ\tNO_CONFL\tACCOUNTS\tUPDATE_RATE\tSIZE_TX\n" > parameters.txt
				for t in 1 2 4 6 8 10 12 14 20 28 40 54 # 1 2 3 4 5 6 # 
				do
					run_bench $j $u $t $NB_ACCOUNTS $NB_TRANSFERS
                    wait ; 
                    pkill bank ;
				done
                wait ;
				mv parameters.txt "$1"_TX"$j"_u"$u"_par_s"$i"
				mv stats_file "$1"_TX"$j"_u"$u"_s"$i"
				mv stats_file.aux_thr "$1"_TX"$j"_u"$u"_aux_s"$i"
			done
		done
	done
}

function bench_updates {
	for i in `seq $SAMPLES` 
	do
		rm -f parameters.txt stats_file stats_file.aux_thr
		TX_SIZE=8
		THREADS=4
		echo -ne "#SEQ\tNO_CONFL\tACCOUNTS\tUPDATE_RATE\tSIZE_TX\n" > parameters.txt
		for u in 5 10 20 30 60 90 100
		do
			run_bench $TX_SIZE $u $THREADS $NB_ACCOUNTS $NB_TRANSFERS
		done
		mv parameters.txt "$1"_par_s"$i"
		mv stats_file "$1"_s"$i"
		mv stats_file.aux_thr "$1"_aux_s"$i"
	done
}

MAKEFILE_ARGS="SOLUTION=1" $BUILD_SCRIPT htm-sgl-nvm HTM >/dev/null
bench_1 HTM_th
bench_updates HTM_up

# reactive
#MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=2 LOG_SIZE=$LOG_SIZE2 THRESHOLD=0.5" \
#    $BUILD_SCRIPT htm-sgl-nvm SOL"2"_l"$LOG_SIZE2"_p"0.0" >/dev/null
#bench_1 SOL"2"_l"$LOG_SIZE2"_p"0.0"_th
#bench_updates SOL"2"_l"$LOG_SIZE2"_p"0.0"_up

### TODO: script merge samples

for s in 4 # TS
do
    for d in 5 # WRAP FORK # 1 PERIODIC
    do
        for l in 500000 # $LOG_SIZE4 #
        do
            for a in 4 # 1 2 3 4 5 # sort-same-thr / array / no-sort / diff-thrs / batch writes
            do
                # TODO: move for a=6
                # a=5
                MAKEFILE_ARGS="SOLUTION=$s DO_CHECKPOINT=$d LOG_SIZE=$l \
                    SORT_ALG=$a THRESHOLD=0.0" \
                    $BUILD_SCRIPT htm-sgl-nvm SOL"$d"_l"$l"_p"$p" >/dev/null
                # a=6
                bench_1 NVHTM_th
                bench_updates NVHTM_up
            done
        done
    done
done


for l in 500000 # $LOG_SIZE4 #
do
    for p in 0.05
    do
        # TODO: move for a=6
        # a=5
        MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=$l \
            THRESHOLD=0.0 SORT_ALG=5 FILTER=$p" \
            $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
        # a=6
        bench_1 NVHTM_FILTER_th
        bench_updates NVHTM_FILTER_up
    done
done

#MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=$LOG_SIZE2 THRESHOLD=$THRESHOLD2" \
#	$BUILD_SCRIPT htm-sgl-nvm FORK_UPDATES_l"$LOG_SIZE2"_p"$THRESHOLD2" >/dev/null
#bench_updates FORK_UPDATES_l"$LOG_SIZE2"_p"$THRESHOLD2"

MAKEFILE_ARGS="SOLUTION=2" $BUILD_SCRIPT htm-sgl-nvm AVNI >/dev/null
bench_1 AVNI_th
bench_updates AVNI_up

