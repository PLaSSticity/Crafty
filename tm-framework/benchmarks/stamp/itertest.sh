#!/bin/sh

backend[1]="backend-seer"
backend[2]="backend-baseline"
backend[3]="backend-hle"
backend[4]="backend-auxlock"

backend[10]="backend-seer-tasklocks"
backend[11]="backend-seer-nohtm"
backend[12]="backend-seer-nogd"

backendMacros[1]="-DUSE_SEER=1 -DUSE_TX_LOCKS=1 -DUSE_CPU_LOCKS=1 -DUSE_HTM_LOCKS=1 -DUSE_GRADIENT_DESCENT=1"
backendMacros[2]=""
backendMacros[3]="-DUSE_HLE"
backendMacros[4]="-DUSE_SCM"

backendMacros[10]="-DUSE_SEER=1 -DUSE_TX_LOCKS=1"
backendMacros[11]="-DUSE_SEER=1 -DUSE_TX_LOCKS=1 -DUSE_CPU_LOCKS=1"
backendMacros[12]="-DUSE_SEER=1 -DUSE_TX_LOCKS=1 -DUSE_CPU_LOCKS=1 -DUSE_HTM_LOCKS=1"

blocks[1]="5"
blocks[2]="3"
blocks[3]="3"
blocks[4]="10"
blocks[5]="3"
blocks[6]="6"
blocks[7]="3"
blocks[8]="3"

benchmarks[1]="genome"
benchmarks[2]="intruder"
benchmarks[3]="kmeans"
benchmarks[4]="ssca2"
benchmarks[5]="vacation"
benchmarks[6]="yada"
benchmarks[7]="kmeans"
benchmarks[8]="vacation"

bStr[1]="genome"
bStr[2]="intruder"
bStr[3]="kmeans-high"
bStr[4]="ssca2"
bStr[5]="vacation-high"
bStr[6]="yada"
bStr[7]="kmeans-low"
bStr[8]="vacation-low"

params[1]="-g16384 -s64 -n16777216 -r1 -t"
params[2]="-a10 -l128 -n262144 -s1 -r1 -t"
params[3]="-m15 -n15 -t0.00001 -i kmeans/inputs/random-n65536-d32-c16.txt -r 1 -p"
params[4]="-s20 -i1.0 -u1.0 -l3 -p3 -r 1 -t"
params[5]="-n4 -q60 -u90 -r1048576 -t4194304 -a 1 -c"
params[6]="-a15 -i yada/inputs/ttimeu1000000.2 -r 1 -t"
params[7]="-m40 -n40 -t0.00001 -i kmeans/inputs/random-n65536-d32-c16.txt -r 1 -p"
params[8]="-n2 -q90 -u98 -r1048576 -t4194304 -a 1 -c"

wait_until_finish() {
    pid3=$1
    echo "process is $pid3"
    LIMIT=120
    for ((j = 0; j < $LIMIT; ++j)); do
        kill -s 0 $pid3
        rc=$?
        if [[ $rc != 0 ]] ; then
            echo "returning"
            return;
        fi
        sleep 1s
    done
    kill -9 $pid3
}

for be in 10 11 12 # {1..4}
do
for retries in 5
do
for apriori in 1
do
    for b in {1..8}
    do
        for t in 8 # {1..8}
        do
            bash build.sh $retries $apriori $t ${blocks[$b]} ${benchmarks[$b]} "${backendMacros[$be]}";
            for a in 1 # {1..20}
            do
                echo "${bStr[$b]} | backend ${backend[$be]} | htm_retries $retries | apriori tries $apriori | threads $t | attempt $a"
                ./${benchmarks[$b]}/${benchmarks[$b]} ${params[$b]}$t > auto-results/${backend[$be]}-${bStr[$b]}-retries$retries-$apriori-th$t-att$a.data &
                pid3=$!; wait_until_finish $pid3; wait $pid3; rc=$?
                if [[ $rc != 0 ]] ; then
                    echo "Error within: backend ${backend[$be]} | ${bStr[$b]} | htm_retries $retries | apriori tries $apriori | threads $t | attempt $a" >> auto-results/error.out
                fi
           done
        done
    done
done
done
done

