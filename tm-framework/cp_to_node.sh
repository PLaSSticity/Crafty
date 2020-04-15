#!/bin/bash

DIR=projs
NAME="tm-framework"
NODE="node30"
CP_TARGET="."

find . -name .DS_* -delete

DM=$DIR/$NAME

if [[ $# -gt 0 ]] ; then
	NODE=$1
fi

if [[ $2 == all ]] ; then
	CP_TARGET="."
fi

if [[ $2 == bench ]] ; then
	CP_TARGET="./benchmarks"
fi

if [[ $2 == backend ]] ; then
	CP_TARGET="./backends"
fi

if [[ $2 == input ]] ; then
	CP_TARGET="./inputs"
fi

if [[ $2 == scripts ]] ; then
	CP_TARGET="./scripts"
fi

tar -czf $NAME.tar.gz $CP_TARGET

ssh $NODE "mkdir -p $DM/ "
scp $NAME.tar.gz $NODE:$DM
ssh $NODE "cd $DM ; gunzip $NAME.tar.gz ; tar -xf $NAME.tar ; rm *.tar* ;" # rm *.tar*
if [[ $2 != bench && $2 != backend && $2 != all ]] ; then
	ssh $NODE "cd $DM ; cp -R ./inputs/* ./benchmarks/stamp-patched/ "
	ssh $NODE "cd $DM ; cp -R ./inputs/* ./benchmarks/stamp-pact/ "
	ssh $NODE "cd $DM ; cp -R ./inputs/* ./benchmarks/stamp-ibm/ "
	ssh $NODE "cd $DM ; cp -R ./inputs/* ./benchmarks/stamp-ibm-mod/ "
fi

rm $NAME.tar.gz
