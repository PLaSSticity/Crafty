#!/bin/bash

#############
# Runs the test suit:
# HTM / PHTM / NV-HTM
#############

SAMPLES=10
NB_BENCH=10

test[1]="./ssca2/ssca2 -s20 -i1.0 -u1.0 -l3 -p3 -t"
test[2]="./kmeans/kmeans -m40 -n40 -t0.05 -i ./kmeans/inputs/random-n16384-d24-c16.txt -p" # low
test[3]="./genome/genome -g512 -s32 -n32768 -t"
test[4]="./vacation/vacation -n2 -q90 -u98 -r1048576 -t4096 -c"  # low
test[5]="./intruder/intruder -a10 -l16 -n4096 -s1 -t"
test[6]="./yada/yada -a10 -i ./yada/inputs/ttimeu10000.2 -t"
test[7]="./vacation/vacation -n4 -q60 -u90 -r1048576 -t4096 -c"  # high
test[8]="./kmeans/kmeans -m15 -n15 -t0.05 -i ./kmeans/inputs/random-n16384-d24-c16.txt -p" # high
test[9]="./labyrinth/labyrinth -i labyrinth/inputs/random-x48-y48-z3-n64.txt -t"
test[10]="./bayes/bayes -v32 -r4096 -n2 -p20 -i2 -e2 -t"

# drop labyrith, it does not had much info

test_name[1]="SSCA2"
test_name[2]="KMEANS_LOW"
test_name[3]="GENOME"
test_name[4]="VACATION_LOW"
test_name[5]="INTRUDER"
test_name[6]="YADA"
test_name[7]="VACATION_HIGH"
test_name[8]="KMEANS_HIGH"
test_name[9]="LABYRINTH"
test_name[10]="BAYES"

THREADS="1 2 4 8 12 13 14 16 22 24 26 27 28"

# 5GB
LOG_SIZE=1000000000

BUILD_SCRIPT=./build-stamp.sh
DATA_FOLDER=./dataSTAMP
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
