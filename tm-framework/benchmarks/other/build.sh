#!/bin/bash
FOLDERS="bank bank-fee simple memory"

if [[ $# -eq 0 ]] ; then
    echo " === ERROR At the very least, we need the backend name in the first parameter. === "
    exit 1
fi

backend=$1  # e.g.: herwl

htm_retries=5
rot_retries=2

if [[ $# -eq 3 ]] ; then
    htm_retries=$2 # e.g.: 5
    rot_retries=$3 # e.g.: 2, this can also be retry policy for tle
fi

rm lib/*.o || true

rm Defines.common.mk
rm Makefile
rm Makefile.flags
rm lib/thread.h
rm lib/thread.c
rm lib/tm.h


cp ../../backends/$backend/Defines.common.mk .
cp ../../backends/$backend/Makefile .
cp ../../backends/$backend/Makefile.flags .
cp ../../backends/$backend/thread.h lib/
cp ../../backends/$backend/thread.c lib/
cp ../../backends/$backend/tm.h lib/

if [[ $backend == htm-sgl-nvm || $backend == htm-sgl-nvm-pact ]] ; then
	PATH_NH=../../../nv-htm
	cd $PATH_NH
	make_command="make -j8 $MAKEFILE_ARGS"
    make clean
    echo "$make_command"
    $make_command
	cd -
fi

if [[ $backend == stm-tinystm ]] ; then
	PATH_TINY=../../../tinystm
	cd $PATH_TINY
	make clean
  make -j8 $MAKEFILE_ARGS
	cd -
fi

for F in $FOLDERS
do
    cd $F
    rm *.o || true
    rm $F
	if [[ $backend == htm-sgl || $backend == htm-sgl-expbf || $backend == hybrid-norec-ptr || $backend == hybrid-norec-opt ]] ; then
		make_command="make -j8 -f Makefile HTM_RETRIES=-DHTM_RETRIES=$htm_retries RETRY_POLICY=-DRETRY_POLICY=$rot_retries"
	else
		make_command="make -j8 -f Makefile HTM_RETRIES=-DHTM_RETRIES=$htm_retries ROT_RETRIES=-DROT_RETRIES=$rot_retries"
	fi
	if [[ $backend == htm-sgl-nvm || $backend == htm-sgl-nvm-pact ]] ; then
		make_command="make -j8 -f Makefile EXTRA_FLAGS=-DSTATS_FILE=\\\"$2\\\" $MAKEFILE_ARGS"
		echo "$make_command"
	fi
    $make_command
    rc=$?
    if [[ $rc != 0 ]] ; then
        echo ""
        echo "=================================== ERROR BUILDING $F - $name ===================================="
        echo ""
        exit 1
    fi
    cd ..
done
