#!/bin/bash

#############
# Compares the backward performance
# (processes data only)
#############

SAMPLES=3

# BANK parameters
NB_TRANSFERS=50000000 #50000000
BANK_BUDGET=1000000
UPDATE_RATE="10 50 90"
THREADS=7
TX_SIZE=2
ITEMS_READ="10 400"

LOG_SIZE=100000

BUILD_SCRIPT=./build.sh
DATA_FOLDER=./dataBACKWARD
SCRIPTS_FOLDER=../../scripts
PLOT_FOLDER=./tests/plot

NB_ACCOUNTS=2048

# dir where build.sh is
cd ..

for r in $ITEMS_READ 
do

for i in `seq $SAMPLES`
do
    for j in NVHTM0_5_"$r" NVHTM_F_"$r" NVHTM_W_"$r" HTM_"$r" PHTM_"$r" 
    do
        $SCRIPTS_FOLDER/SCRIPT_concat_files.R \
            `ls $DATA_FOLDER/"$j"_*"$i"`
        mv cat.txt $DATA_FOLDER/"$j"_s_"$i"CAT
    done
done

done

$SCRIPTS_FOLDER/SCRIPT_put_column.R "\
            UPDATE_RATE = a\$UPDATE_RATE; \
			PA = a\$ABORTS / (a\$ABORTS + a\$COMMITS_HTM); \
			P_CON = a\$CONFLICT / a\$ABORTS * PA; \
			P_CAP = a\$CAPACITY / a\$ABORTS * PA; \
			P_OTH = PA - P_CON - P_CAP; \
			THROUGHPUT = a\$NB_TXS / a\$TIME; \
			PERC_BLOCKED = a\$PERC_BLOCKED; \
            NB_FLUSHES = a\$NB_FLUSHES; \
			a = cbind(UPDATE_RATE, PA, P_CON, P_CAP, P_OTH, THROUGHPUT, \
                PERC_BLOCKED, NB_FLUSHES); " \
			`ls $DATA_FOLDER/HTM*CAT $DATA_FOLDER/PHTM*CAT`

$SCRIPTS_FOLDER/SCRIPT_put_column.R "\
            UPDATE_RATE = a\$UPDATE_RATE; \
			PA = a\$ABORTS / (a\$ABORTS + a\$COMMITS_HTM); \
			P_CON = a\$CONFLICT / a\$ABORTS * PA; \
			P_CAP = a\$CAPACITY / a\$ABORTS * PA; \
			P_OTH = PA - P_CON - P_CAP; \
			THROUGHPUT = a\$NB_TXS / a\$TIME; \
			PERC_BLOCKED = a\$PERC_BLOCKED; \
            NB_FLUSHES = as.numeric(a\$NB_FLUSHES) + as.numeric(a\$X.NB_FLUSHES); \
			a = cbind(UPDATE_RATE, PA, P_CON, P_CAP, P_OTH, THROUGHPUT, \
                PERC_BLOCKED, NB_FLUSHES); " \
			`ls $DATA_FOLDER/NVHTM*CAT`

for r in $ITEMS_READ 
do

# NVHTM0_01 NVHTM0_05 NVHTM0_1 NVHTM0_9
for j in NVHTM0_5_"$r" NVHTM_F_"$r" NVHTM_W_"$r" HTM_"$r" PHTM_"$r" 
do
    $SCRIPTS_FOLDER/SCRIPT_compute_AVG_ERR.R \
        `ls $DATA_FOLDER/"$j"*CAT.cols`
    mv avg.txt $DATA_FOLDER/test_"$j"_BACK.txt
done

gnuplot -c $PLOT_FOLDER/test_BACK.gp            $DATA_FOLDER $NB_ACCOUNTS $THREADS $r
gnuplot -c $PLOT_FOLDER/test_BACK_PREC.gp       $DATA_FOLDER $NB_ACCOUNTS $THREADS $r
gnuplot -c $PLOT_FOLDER/test_BACK_PA.gp         $DATA_FOLDER $NB_ACCOUNTS $THREADS $r
gnuplot -c $PLOT_FOLDER/test_BACK_NB_FLUSHES.gp $DATA_FOLDER $NB_ACCOUNTS $THREADS $r

done

#rm $DATA_FOLDER/*CAT $DATA_FOLDER/*CAT.cols
