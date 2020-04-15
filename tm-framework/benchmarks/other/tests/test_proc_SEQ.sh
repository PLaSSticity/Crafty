#!/bin/bash

#############
# Compares the maximum capacity of:
# HTM / PHTM / NV-HTM
#
#############

SAMPLES=10

# BANK parameters
NB_TRANSFERS=1000000
BANK_BUDGET=100000
UPDATE_RATE="1 10 50 90 99"
THREADS="14"
TX_SIZE="1"
READ_SIZE="64"

LOG_SIZE=100000

BUILD_SCRIPT=./build.sh
DATA_FOLDER=./dataSEQ
SCRIPTS_FOLDER=../../scripts
PLOT_FOLDER=./tests/plot
NAME="SEQ"

NB_ACCOUNTS=8192

# dir where build.sh is
cd ..

for i in `seq $SAMPLES`
do
    for j in HTM NVHTM NVHTM_LC PHTM NVHTM_W NVHTM_LC_W
    do
        $SCRIPTS_FOLDER/SCRIPT_concat_files.R \
            `ls $DATA_FOLDER/"$j"_par_s"$i" \
                $DATA_FOLDER/"$j"_s"$i" \
                $DATA_FOLDER/"$j"_aux_s"$i"`
        mv cat.txt $DATA_FOLDER/"$j"_s_"$i"CAT
    done
done
$SCRIPTS_FOLDER/SCRIPT_put_column.R "\
      UPDATE_RATE = a\$UPDATE_RATE; \
			PA = a\$ABORTS / (a\$ABORTS + a\$COMMITS_HTM); \
			P_CON = a\$CONFLICT / a\$ABORTS * PA; \
			P_CAP = a\$CAPACITY / a\$ABORTS * PA; \
			P_OTH = PA - P_CON - P_CAP; \
			THROUGHPUT = a\$NB_TXS / a\$TIME; \
			a = cbind(UPDATE_RATE, PA, P_CON, P_CAP, P_OTH, THROUGHPUT); " \
			`ls $DATA_FOLDER/*CAT`

for j in HTM NVHTM NVHTM_LC PHTM NVHTM_W NVHTM_LC_W
do
    $SCRIPTS_FOLDER/SCRIPT_compute_AVG_ERR.R \
        `ls $DATA_FOLDER/"$j"*CAT.cols`
    mv avg.txt $DATA_FOLDER/test_"$j"_"$NAME".txt
done

# TODO
gnuplot -c $PLOT_FOLDER/test_SEQ_X.gp  "$NAME" $DATA_FOLDER $NB_ACCOUNTS $TX_SIZE $READ_SIZE $THREADS
gnuplot -c $PLOT_FOLDER/test_SEQ_PA.gp "$NAME" $DATA_FOLDER $NB_ACCOUNTS $TX_SIZE $READ_SIZE $THREADS

rm $DATA_FOLDER/*CAT $DATA_FOLDER/*CAT.cols
