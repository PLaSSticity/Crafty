#!/bin/sh


benchmarks[1]="-s 96 -d 1 -o 1 -p 1 -r 1"
benchmarks[2]="-s 1 -d 96 -o 1 -p 1 -r 1"
benchmarks[3]="-s 1 -d 1 -o 96 -p 1 -r 1"
benchmarks[4]="-s 1 -d 1 -o 1 -p 96 -r 1"
benchmarks[5]="-s 1 -d 1 -o 1 -p 1 -r 96"
benchmarks[6]="-s 20 -d 20 -o 20 -p 20 -r 20"
benchmarks[7]="-s 4 -d 4 -o 4 -p 43 -r 45"
benchmarks[10]="-m 32 -c 50 -w 1 -s 0 -d 0 -o 100 -p 0 -r 0 -c 50 -w 1 -s 4 -d 4 -o 4 -p 43 -r 45 -c 50 -w 32 -s 96 -d 1 -o 1 -p 1 -r 1"

export LD_LIBRARY_PATH=/home/ndiegues/boost_1_57_0/stage/lib/;
./code/tpcc -t 5 -n 1 -w 1 ${benchmarks[1]}

