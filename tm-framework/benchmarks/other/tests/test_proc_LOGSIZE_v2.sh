#!/bin/bash

#############
# Compares the backward performance
# (processes data only)
#############

SAMPLES=10

# BANK parameters
NB_TRANSFERS=5000000
BANK_BUDGET=1000000
UPDATE_RATE="90"
THREADS="1 2 4 8 12 16 20 24 28"
TX_SIZE="1"
READ_SIZE="16"

LOG_SIZE="25000000 150000000 250000000"

BUILD_SCRIPT=./build.sh
DATA_FOLDER_10_N=./dataLOGSIZE2_10_N
DATA_FOLDER_90_N=./dataLOGSIZE2_90_N
DATA_FOLDER_10_C=./dataLOGSIZE2_10_C
DATA_FOLDER_90_C=./dataLOGSIZE2_90_C
SCRIPTS_FOLDER=../../scripts
PLOT_FOLDER=./tests/plot


NB_ACCOUNTS=2048

# dir where build.sh is
cd ..


for i in `seq $SAMPLES`
do
    for l in $LOG_SIZE
    do
        for j in NVHTM_B_50_"$l" NVHTM_F_"$l"
        do
            $SCRIPTS_FOLDER/SCRIPT_concat_files.R \
                `ls $DATA_FOLDER_10_N/"$j"_*"$i"`
            mv cat.txt $DATA_FOLDER_10_N/"$j"_s_"$i"CAT
            $SCRIPTS_FOLDER/SCRIPT_concat_files.R \
                `ls $DATA_FOLDER_90_N/"$j"_*"$i"`
            mv cat.txt $DATA_FOLDER_90_N/"$j"_s_"$i"CAT
            $SCRIPTS_FOLDER/SCRIPT_concat_files.R \
                `ls $DATA_FOLDER_10_C/"$j"_*"$i"`
            mv cat.txt $DATA_FOLDER_10_C/"$j"_s_"$i"CAT
            $SCRIPTS_FOLDER/SCRIPT_concat_files.R \
                `ls $DATA_FOLDER_90_C/"$j"_*"$i"`
            mv cat.txt $DATA_FOLDER_90_C/"$j"_s_"$i"CAT
        done
    done
    $SCRIPTS_FOLDER/SCRIPT_concat_files.R \
        `ls $DATA_FOLDER_10_N/HTM_*"$i"`
    $SCRIPTS_FOLDER/SCRIPT_concat_files.R \
        `ls $DATA_FOLDER_10_N/PHTM_*"$i"`
    $SCRIPTS_FOLDER/SCRIPT_concat_files.R \
        `ls $DATA_FOLDER_10_N/NVHTM_W_*"$i"`
done
$SCRIPTS_FOLDER/SCRIPT_put_column.R "\
			UPDATE_RATE = a\$UPDATE_RATE; \
			PA = a\$ABORTS / (a\$ABORTS + a\$COMMITS_HTM); \
			P_CON = a\$CONFLICT / a\$ABORTS * PA; \
			P_CAP = a\$CAPACITY / a\$ABORTS * PA; \
			P_EXP = a\$EXPLICIT / a\$ABORTS * PA; \
			P_OTH = PA - P_CON - P_CAP - P_EXP; \
			THROUGHPUT = a\$NB_TXS / a\$TIME; \
			PERC_BLOCKED = a\$PERC_BLOCKED; \
            NB_FLUSHES_CHKP = a\$X.NB_FLUSHES; \
            NB_FLUSHES = a\$NB_FLUSHES; \
            REMAIN = a\$REMAIN_LOG; \
            NB_BLOCKS = a\$REMAIN_LOG; \
			a = cbind(UPDATE_RATE, PA, P_CON, P_CAP, P_EXP, P_OTH, THROUGHPUT, \
                PERC_BLOCKED, NB_FLUSHES_CHKP, NB_BLOCKS, REMAIN); " \
			`ls $DATA_FOLDER_10_N/*CAT $DATA_FOLDER_90_N/*CAT \
                $DATA_FOLDER_90_C/*CAT $DATA_FOLDER_90_C/*CAT`

for l in $LOG_SIZE
do
    for j in NVHTM_B_50_"$l" NVHTM_F_"$l"
    do
        $SCRIPTS_FOLDER/SCRIPT_compute_AVG_ERR.R \
            `ls $DATA_FOLDER/"$j"*CAT.cols`
        mv avg.txt $DATA_FOLDER/test_"$j".txt
    done
done
$SCRIPTS_FOLDER/SCRIPT_compute_AVG_ERR.R \
    `ls $DATA_FOLDER/PHTM*CAT.cols`
mv avg.txt $DATA_FOLDER/test_PHTM.txt
$SCRIPTS_FOLDER/SCRIPT_compute_AVG_ERR.R \
    `ls $DATA_FOLDER/HTM*CAT.cols`
mv avg.txt $DATA_FOLDER/test_HTM.txt
$SCRIPTS_FOLDER/SCRIPT_compute_AVG_ERR.R \
    `ls $DATA_FOLDER/NVHTM_W*CAT.cols`
mv avg.txt $DATA_FOLDER/test_NVHTM_W.txt

# TODO: TOO MANY LINES!
for u in "10" "90"
do
    for c in 'C' 'N'
    do
        gnuplot -c $PLOT_FOLDER/test_LOGSIZE2_X.gp          LOGSIZE"$u""$c" ./dataLOGSIZE2_"$u"_"$c" $NB_ACCOUNTS $TX_SIZE $u $READ_SIZE $NB_TXS
        gnuplot -c $PLOT_FOLDER/test_LOGSIZE2_BLOCKS.gp     LOGSIZE"$u""$c" ./dataLOGSIZE2_"$u"_"$c" $NB_ACCOUNTS $TX_SIZE $u $READ_SIZE $NB_TXS
        gnuplot -c $PLOT_FOLDER/test_LOGSIZE2_TIME_BLOCK.gp LOGSIZE"$u""$c" ./dataLOGSIZE2_"$u"_"$c" $NB_ACCOUNTS $TX_SIZE $u $READ_SIZE $NB_TXS
        gnuplot -c $PLOT_FOLDER/test_LOGSIZE2_FLUSHES.gp    LOGSIZE"$u""$c" ./dataLOGSIZE2_"$u"_"$c" $NB_ACCOUNTS $TX_SIZE $u $READ_SIZE $NB_TXS
        gnuplot -c $PLOT_FOLDER/test_LOGSIZE2_REM.gp        LOGSIZE"$u""$c" ./dataLOGSIZE2_"$u"_"$c" $NB_ACCOUNTS $TX_SIZE $u $READ_SIZE $NB_TXS
        rm ./dataLOGSIZE2_"$u"_"$c"/*CAT ./dataLOGSIZE2_"$u"_"$c"/*CAT.cols
    done
done
