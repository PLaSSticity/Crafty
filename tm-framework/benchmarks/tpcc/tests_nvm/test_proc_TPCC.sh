#!/bin/bash

#############
# Runs the test suit:
# HTM / PHTM / NV-HTM
#############
SAMPLES=20
NB_BENCH=1

test[1]="./code/tpcc -t 5 -w 1 -s 1 -d 4 -o 90 -p 4 -r 1 -n"
#test[1]="./code/tpcc -t 5 -w 1 -s 0 -d 5 -o 90 -p 5 -r 0 -n"

# drop labyrith, it does not had much info

test_name[1]="TPCC"

THREADS="1 2 4 8 12 16 20 24 28"

# 1GB
LOG_SIZE=1000000000

BUILD_SCRIPT=./build-tpcc.sh
DATA_FOLDER=./dataTPCC
SCRIPTS_FOLDER=../../scripts
PLOT_FOLDER=./tests_nvm/plot
# dir where build.sh is
cd ..

for i in `seq $SAMPLES`
do
    for j in HTM NVHTM_W NVHTM_F NVHTM_B PHTM STM PSTM
    do
        for k in `seq $NB_BENCH`
        do
            $SCRIPTS_FOLDER/SCRIPT_concat_files.R \
                `ls $DATA_FOLDER/"${test_name[$k]}"_"$j"_s"$i" \
                    $DATA_FOLDER/"${test_name[$k]}"_"$j"_aux_s"$i" \
                    $DATA_FOLDER/"${test_name[$k]}"_"$j"_par_s"$i"`
            mv cat.txt $DATA_FOLDER/"${test_name[$k]}"_"$j"_"$i"CAT
        done
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
			`ls $DATA_FOLDER/*HTM*CAT`

$SCRIPTS_FOLDER/SCRIPT_put_column.R "\
            THREADS = a\$X.THREADS; \
			PA = a\$ABORTS / (a\$ABORTS + a\$COMMITS); \
			NB_FLUSHES = a\$NB_FLUSHES; \
            FLUSHES_PER_TX = a\$NB_FLUSHES / a\$COMMITS; \
			THROUGHPUT = a\$COMMITS / a\$TIME; \
			a = cbind(THREADS, PA, PA, NB_FLUSHES, FLUSHES_PER_TX, THROUGHPUT); " \
			`ls $DATA_FOLDER/*STM*CAT`

for j in HTM NVHTM_W NVHTM_F NVHTM_B PHTM STM PSTM
do
    for k in `seq $NB_BENCH`
    do
        $SCRIPTS_FOLDER/SCRIPT_compute_AVG_ERR.R \
            `ls $DATA_FOLDER/"${test_name[$k]}"_"$j"*CAT.cols`
        mv avg.txt $DATA_FOLDER/test_"${test_name[$k]}"_"$j".txt
    done
done

for k in `seq $NB_BENCH`
do
    gnuplot -c $PLOT_FOLDER/plot_X.gp  $DATA_FOLDER "${test_name[$k]}" "${test[$k]}"
    gnuplot -c $PLOT_FOLDER/plot_PA.gp $DATA_FOLDER "${test_name[$k]}" "${test[$k]}"
done

rm $DATA_FOLDER/*CAT $DATA_FOLDER/*.cols
