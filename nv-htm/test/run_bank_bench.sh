#!/bin/bash

SAMPLES=10
NB_TRANSFERS=1000000
BANK_BUDGET=1000000
	
LOG_SMALL=10000
LOG_LARGE=1000000

# TODO: run this
# > ipcrm -M 0x00054321
# to remove the log shared segment

run_capacity() {

	./compile.sh $1 $2 $3

	for i in `seq $SAMPLES`
	do
		for t in 1
		do
			for s in 1 2 4 8 16 32 48 64 96 128 256
			do
				for a in 8192
				do
					rm -f /tmp/*.socket
					ipcrm -M 0x00054321 # kills the shared memory log segment
					timeout 5m ./bank THREADS $t NB_ACCOUNTS $a SEQ 1 \
						NB_TRANSFERS 100000 TXS $s BANK_BUDGET $BANK_BUDGET \
						GNUPLOT_FILE sample"$i"_CAP_"$1".txt >/dev/null
				done
			done
		done
	done
}

run_no_confl() {
	./compile.sh $1 $2 $3

	for i in `seq $SAMPLES`
	do
		for t in 2 4 8 10 12 14 15 28 56
		do
			for s in 1 2 4 8 16 32 48 64 96 128 256
			do
				for a in 8192
				do
					rm -f /tmp/*.socket
					ipcrm -M 0x00054321 # kills the shared memory log segment
					timeout 5m ./bank THREADS $t NB_ACCOUNTS $a SEQ 1 NO_CONFL 1 \
						NB_TRANSFERS 100000 TXS $s BANK_BUDGET $BANK_BUDGET \
						GNUPLOT_FILE sample"$i"_thrs"$t"_NO_CONFL_"$1".txt >/dev/null
				done
			done
		done
	done
}

run_log_size() {
	threads=4
	accounts=1024
	txs=4
	# s --> REACTIVE WRAP FORK 
	for s in 2 4 5
	do
		for l in 1000 10000 100000 1000000
		do
			for p in 0.01 0.1 0.5 0.9 0.99
			do
				make clean && make -j56 SOLUTION=4 OPT="-O0" LOG_SIZE=$l \
					DO_CHECKPOINT=$s THRESHOLD=$p
				
				for i in `seq $SAMPLES`
				do
					for u in 1
					do
						rm -f /tmp/*.socket
						ipcrm -M 0x00054321 # kills the shared memory log segment
						timeout 8m ./bank THREADS $threads NB_ACCOUNTS $accounts \
							SEQ 1 NO_CONFL 0 NB_TRANSFERS 10000000 TXS $txs \
							BANK_BUDGET $BANK_BUDGET UPDATE_RATE $u\
							GNUPLOT_FILE sample"$i"_STRESS_LOG_"$l"_SOL"$s"_U"$u".txt >/dev/null
					done
				done
			done
		done
	done
}

run_batch() {

	./compile.sh $1 $2 $3

	for t in 1 2 4 8 10 12 14 15 28 56
	do
		if [[ $2 == "FORK" ]] ; then
			CHANGE_LOG_SIZE=$((1000 * 10**(t-1)))
			if [[ $CHANGE_LOG_SIZE -gt 1000000 ]] ; then
				CHANGE_LOG_SIZE=1000000
				echo "LOG SIZE $CHANGE_LOG_SIZE"
			fi
			./compile.sh $1 $2 $CHANGE_LOG_SIZE
		fi
		for i in `seq $SAMPLES`
		do
			for a in 1024 8192
			do
				for s in 1 8 
				do
					rm -f /tmp/*.socket
					ipcrm -M 0x00054321 # kills the shared memory log segment
					timeout 5m ./bank THREADS $t NB_ACCOUNTS $a \
						NB_TRANSFERS $NB_TRANSFERS TXS $s BANK_BUDGET $BANK_BUDGET \
						GNUPLOT_FILE sample"$i"_txs"$s"_BATCH1_"$1"_"$2"_LOG"$3".txt >/dev/null
				done
			done
		done
	done
}

run_batch_no_confl() {

	./compile.sh $1 $2 $3

	for i in `seq $SAMPLES`
	do
		for t in 1 2 4 8 10 12 14 15 28 56
		do
			for s in 1 2 4 8 16 32 48 64 96 128 256
			do
				for a in 512
				do
					rm -f /tmp/*.socket
					ipcrm -M 0x00054321 # kills the shared memory log segment
					timeout 5m ./bank THREADS $t NB_ACCOUNTS $a SEQ 1 NO_CONFL 1 \
						NB_TRANSFERS $NB_TRANSFERS TXS $s BANK_BUDGET $BANK_BUDGET \
						GNUPLOT_FILE sample"$i"_NO_CONFL_"$1".txt >/dev/null
				done
			done
		done
	done
}

run_batch_stress_rdtsc() {

	./compile.sh $1 NO_MANAGER 10000000

	for i in `seq $SAMPLES`
	do
		for t in 2 4 8 # 14 28 56
		do
			for s in 1 2 4 8
			do
				for a in 100 1000
				do
					rm -f /tmp/*.socket
					ipcrm -M 0x00054321 # kills the shared memory log segment
					timeout 5m ./bank THREADS $t NB_ACCOUNTS $a \
						NB_TRANSFERS $NB_TRANSFERS TXS $s BANK_BUDGET $BANK_BUDGET \
						GNUPLOT_FILE sample"$i"_accounts"$a"_"$1".txt >/dev/null
				done
			done
		done
	done
}

## STRESS CAPACITY # TODO::::
# SAMPLES=30
# run_capacity HTM
# run_capacity AVNI
# run_capacity REDO_TS WRAP $LOG_LARGE
# run_capacity REDO_COUNTER WRAP $LOG_LARGE
SAMPLES=5
# run_no_confl HTM
# run_no_confl AVNI
# run_no_confl REDO_TS WRAP $LOG_LARGE
# run_no_confl REDO_COUNTER WRAP $LOG_LARGE
# run_no_confl REDO_TS FORK $LOG_LARGE
# run_no_confl REDO_COUNTER FORK $LOG_LARGE
# run_no_confl REDO_TS FORK $LOG_SMALL # TODO: is merging with the prev ones
# run_no_confl REDO_COUNTER FORK $LOG_SMALL # TODO: is merging with the prev ones

run_batch HTM
run_batch AVNI
### run_batch REDO_TS REACTIVE $LOG_SMALL
run_batch REDO_TS REACTIVE $LOG_LARGE
### run_batch REDO_TS PERIODIC $LOG_SMALL
### run_batch REDO_TS PERIODIC $LOG_LARGE
### run_batch REDO_COUNTER REACTIVE $LOG_SMALL
run_batch REDO_COUNTER REACTIVE $LOG_LARGE
### run_batch REDO_COUNTER PERIODIC $LOG_SMALL
### run_batch REDO_COUNTER PERIODIC $LOG_LARGE
# run_batch REDO_COUNTER FORK $LOG_LARGE
# run_batch REDO_TS FORK $LOG_LARGE

# run_log_size
