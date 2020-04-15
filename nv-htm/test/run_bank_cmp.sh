#!/bin/bash

SAMPLES=5
NB_TRANSFERS=500000
BANK_BUDGET=100000
	
run_batch() {

	./compile.sh $1

	for i in `seq $SAMPLES`
	do
		for t in 1 2 4 6 8 10 12 14 15 28 29 42 43 56
		do
			for s in 4 8 16 32 48 64 96 128 
			do
				for a in 2048 8192 32768
				do
					timeout 1m ./bank THREADS $t NB_ACCOUNTS $a \
						NB_TRANSFERS $NB_TRANSFERS TXS $s BANK_BUDGET $BANK_BUDGET \
						GNUPLOT_FILE sample"$i"_accounts"$a"_"$1".txt >/dev/null
				done
			done
		done
	done
}

run_batch_no_confl() {

	./compile.sh $1

	for i in `seq $SAMPLES`
	do
		for t in 1 2 4 6 8 10 12 14 15 28 29 42 43 56
		do
			for s in 4 8 16 32 48 64 96 128 
			do
				for a in 512
				do
					timeout 1m ./bank THREADS $t NB_ACCOUNTS $a NO_CONFL 1 \
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
		for t in 2 4 8 14 28 56
		do
			for s in 1 2 4 8
			do
				for a in 100 1000
				do
					timeout 1m ./bank THREADS $t NB_ACCOUNTS $a \
						NB_TRANSFERS $NB_TRANSFERS TXS $s BANK_BUDGET $BANK_BUDGET \
						GNUPLOT_FILE sample"$i"_accounts"$a"_"$1".txt >/dev/null
				done
			done
		done
	done
}

# run_batch_no_confl "HTM"
# run_batch_no_confl "AVNI"
# run_batch_no_confl "REDO_TS REACTIVE"
# run_batch_no_confl "REDO_COUNTER REACTIVE"
# run_batch_no_confl "REDO_TS PERIODIC"
# run_batch_no_confl "REDO_COUNTER PERIODIC"

#run_batch "HTM"
#run_batch "AVNI"
# run_batch "REDO_TS REACTIVE"
# run_batch "REDO_COUNTER REACTIVE"
# run_batch "REDO_TS PERIODIC"
# run_batch "REDO_COUNTER PERIODIC"

# sanity test
run_batch_stress_rdtsc "REDO_TS"
