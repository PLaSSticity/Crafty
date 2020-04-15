#/bin/bash

SAMPLES=10
THREADS=`seq 1 14 && seq 16 4 56`

rm -f *.txt # WARNG: removes all .txt files

BENCH=./intruder/intruder
# -a10 -l128 -n10000 -s1 -r1 -t$n	
BENCH=./ssca2/ssca2
# -s300 -i0.5 -u0.7 -l5 -p2 -r1 -t$n

for s in `seq 1 $SAMPLES`
do
	# ./build-nvmhtm.sh normal normal_$s.txt 
	# #echo '# Core | IPC  | Instructions | Cycles  | Transactional Cycles | Aborted Cycles  | #RTM  | #HLE  | Cycles/Transaction' > normal_TSX_$s.txt
	# for u in 0 10 50 90 100
	# do
		# for n in `seq 1 14 && seq 16 4 56`
		# do
			# nice -n 19 timeout 1m ./hashmap/hashmap -n$n -d199999 -u$u -i99999 -r9999999	
		# done
	# done 
	
	./build-nvmhtm.sh normal_no_val normal_no_val_$s.txt 
	###echo '# Core | IPC  | Instructions | Cycles  | Transactional Cycles | Aborted Cycles  | #RTM  | #HLE  | Cycles/Transaction' > normal_TSX_$s.txt
	for u in 0 # TODO
	do
		for n in $THREADS
		do
			nice -n 19 timeout 1m $BENCH -s300 -i0.5 -u0.7 -l5 -p2 -r1 -t$n
		done
	done 

	./build-nvmhtm.sh ch_p_val_2 ch_p_val_2_$s.txt 10000
	# echo '# Core | IPC  | Instructions | Cycles  | Transactional Cycles | Aborted Cycles  | #RTM  | #HLE  | Cycles/Transaction' > normal_TSX_$s.txt
	for u in 0 # TODO
	do
		for n in $THREADS
		do
			nice -n 19 timeout 1m $BENCH -s300 -i0.5 -u0.7 -l5 -p2 -r1 -t$n
		done
	done 
	
	./build-nvmhtm.sh ch_r_val_2 ch_r_val_2_$s.txt 10000
	# echo '# Core | IPC  | Instructions | Cycles  | Transactional Cycles | Aborted Cycles  | #RTM  | #HLE  | Cycles/Transaction' > normal_TSX_$s.txt
	for u in 0 # TODO
	do
		for n in $THREADS
		do
			nice -n 19 timeout 1m $BENCH -s300 -i0.5 -u0.7 -l5 -p2 -r1 -t$n
		done
	done 
	
	./build-nvmhtm.sh ch_p_val_3 ch_p_val_3_$s.txt 10000
	# echo '# Core | IPC  | Instructions | Cycles  | Transactional Cycles | Aborted Cycles  | #RTM  | #HLE  | Cycles/Transaction' > normal_TSX_$s.txt
	for u in 0 # TODO
	do
		for n in $THREADS
		do
			nice -n 19 timeout 1m $BENCH -s300 -i0.5 -u0.7 -l5 -p2 -r1 -t$n
		done
	done 
	
	# ./build-nvmhtm.sh ch_r_val_3 ch_r_val_3_$s.txt 10000
	##### echo '# Core | IPC  | Instructions | Cycles  | Transactional Cycles | Aborted Cycles  | #RTM  | #HLE  | Cycles/Transaction' > normal_TSX_$s.txt
	# for u in 0 # TODO
	# do
		# for n in $THREADS
		# do
			# nice -n 19 timeout 1m $BENCH -a10 -l128 -n10000 -s1 -r1 -t$n	
		# done
	# done 
	
	# ./build-nvmhtm.sh normal_no_val normal_no_val_$s.txt 
	# for u in 0 10 50 90 100
	# do
		# for n in `seq 1 14 && seq 16 4 56`
		# do
			# nice -n 19 timeout 1m ./hashmap/hashmap -n$n -d199999 -u$u -i99999 -r9999999 
		# done
	# done

	# ./build-nvmhtm.sh normal_no_val_no_cnt normal_no_val_no_cnt_$s.txt 
	######echo '# Core | IPC  | Instructions | Cycles  | Transactional Cycles | Aborted Cycles  | #RTM  | #HLE  | Cycles/Transaction' > normal_no_val_TSX_$s.txt
	# for u in 0 10 50 90 100
	# do
		# for n in `seq 1 14 && seq 16 4 56`
		# do
			# nice -n 19 timeout 1m ./hashmap/hashmap -n$n -d199999 -u$u -i99999 -r9999999 
		# done
	# done
	
	./build-nvmhtm.sh avni avni_$s.txt 
	for u in 0 # TODO
	do
		for n in $THREADS
		do
			nice -n 19 timeout 1m $BENCH -s300 -i0.5 -u0.7 -l5 -p2 -r1 -t$n
		done
	done

	./build-nvmhtm.sh htm_only htm_only_$s.txt 
	for u in 0 # TODO
	do
		for n in $THREADS
		do
			nice -n 19 timeout 1m $BENCH -s300 -i0.5 -u0.7 -l5 -p2 -r1 -t$n
			####sudo /usr/local/bin/pcm/pcm-tsx.x -- nice -n 19 timeout 1m ./hashmap/hashmap -n$n -u$u -d199999 -i99999 | grep -P '\*' >> htm_only_TSX_$s.txt
		done
	done
done
