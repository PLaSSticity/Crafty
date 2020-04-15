#!/usr/bin/env bash
backend=$1 # e.g: herwl
htm_retries=$2 # e.g.: 5
rot_retries=$3 # e.g.: 2

cp ../../backends/$backend/tm.h ./include/
cp ../../backends/$backend/thread.c ./src/
cp ../../backends/$backend/thread.h ./include/
cp ../../backends/$backend/Makefile .
cp ../../backends/$backend/Makefile.common .
cp ../../backends/$backend/Makefile.flags .
cp Makefile.common ..
cp Makefile.flags ..
cp ../../backends/$backend/Defines.common.mk .

rm $(find . -name *.o)

cd code;
rm tpcc

if [[ $backend == htm-sgl-nvm ]] ; then
	PATH_NH=../../../nvm-emulation
	cd $PATH_NH
	make clean && make $MAKEFILE_ARGS
	cd -
fi

if [[ $backend == stm-tinystm ]] ; then
	PATH_TINY=../../../tinystm
	cd $PATH_TINY
	make clean
  make $MAKEFILE_ARGS
	cd -
fi

make_command="make -j8 -f Makefile $MAKEFILE_ARGS"
echo " ==========> $make_command"
$make_command
