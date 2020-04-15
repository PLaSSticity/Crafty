#!/bin/bash

DIR=projs
NAME="arch_dep"
NODE="node30"

DM=$DIR/$NAME

if [[ $# -gt 0 ]] ; then
	NODE=$1
fi

tar -czf $NAME.tar.gz .

ssh $NODE "mkdir $DIR ; mkdir $DM "
scp $NAME.tar.gz $NODE:$DM
ssh $NODE "cd $DM ; gunzip $NAME.tar.gz ; tar -xf $NAME.tar ; \
      rm *.tar *.tar.gz ; "

rm $NAME.tar.gz
