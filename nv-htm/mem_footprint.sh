#!/bin/bash

./compile.sh REDO_COUNTER FORK 100

echo "" > fork_test.txt

./bank NB_TRANSFERS 25000000 BANK_BUDGET 999999999 NB_ACCOUNTS 10000000 THREADS 8 TXS 2 &

for i in `seq 60`
do
	echo "at $i seconds\n" >> fork_test.txt 
    free -m >> fork_test.txt
	echo -n "\n\n" >> fork_test.txt 
    sleep 1
done

#######################################

./compile.sh REDO_COUNTER REACTIVE 100

echo "" > reactive_test.txt

./bank NB_TRANSFERS 20000000 BANK_BUDGET 999999999 NB_ACCOUNTS 10000000 THREADS 8 TXS 2 &

for i in `seq 60`
do
	echo -n "at $i seconds\n" >> reactive_test.txt 
    free -m >> reactive_test.txt
	echo -n "\n\n" >> reactive_test.txt 
    sleep 1
done
