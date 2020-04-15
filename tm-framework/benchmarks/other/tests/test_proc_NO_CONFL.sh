#!/bin/bash

#############
# Compares the maximum capacity of:
# HTM / PHTM / NV-HTM
#
#############

SAMPLES=20

# BANK parameters
NB_TRANSFERS=10000000
BANK_BUDGET=100000
UPDATE_RATE=100
THREADS="1 2 4 6 8 12 16 20 24 28"
TX_SIZE="1"

LOG_SIZE=100000000

BUILD_SCRIPT=./build.sh
DATA_FOLDER=./dataNOCONFL
SCRIPTS_FOLDER=../../scripts
PLOT_FOLDER=./tests/plot
NAME="NOCONFL"

NB_ACCOUNTS=1024

# dir where build.sh is
cd ..

for i in `seq $SAMPLES`
do
    for j in HTM NVHTM NVHTM_LC PHTM NVHTM_W NVHTM_LC_W NVHTM_B NVHTM_LC_B
    do
        $SCRIPTS_FOLDER/SCRIPT_concat_files.R \
            `ls $DATA_FOLDER/"$j"_par_s"$i" \
                $DATA_FOLDER/"$j"_s"$i" \
                $DATA_FOLDER/"$j"_aux_s"$i"`
        mv cat.txt $DATA_FOLDER/"$j"_s_"$i"CAT
    done
done
$SCRIPTS_FOLDER/SCRIPT_put_column.R "\
            THREADS = a\$X.THREADS; \
			PA = a\$ABORTS / (a\$ABORTS + a\$COMMITS_HTM); \
			P_CON = a\$CONFLICT / a\$ABORTS * PA; \
			P_CAP = a\$CAPACITY / a\$ABORTS * PA; \
			P_OTH = PA - P_CON - P_CAP; \
			THROUGHPUT = a\$NB_TXS / a\$TIME; \
			a = cbind(THREADS, PA, P_CON, P_CAP, P_OTH, THROUGHPUT); " \
			`ls $DATA_FOLDER/*CAT`

for j in HTM NVHTM NVHTM_LC PHTM NVHTM_W NVHTM_LC_W NVHTM_B NVHTM_LC_B
do
    $SCRIPTS_FOLDER/SCRIPT_compute_AVG_ERR.R \
        `ls $DATA_FOLDER/"$j"*CAT.cols`
    mv avg.txt $DATA_FOLDER/test_"$j"_"$NAME".txt
done

# TODO
gnuplot -c $PLOT_FOLDER/test_NOCONFL_X.gp  "$NAME" $DATA_FOLDER $NB_ACCOUNTS $TX_SIZE
gnuplot -c $PLOT_FOLDER/test_NOCONFL_PA.gp "$NAME" $DATA_FOLDER $NB_ACCOUNTS $TX_SIZE

rm $DATA_FOLDER/*CAT $DATA_FOLDER/*CAT.cols
