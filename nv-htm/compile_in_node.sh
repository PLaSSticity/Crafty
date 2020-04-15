#!/bin/bash

DIR=projs
NAME="nh"
NODE="node30"

DM=$DIR/$NAME

find . -name .DS_* -delete

if [[ $# -gt 0 ]] ; then
	NODE=$1
fi

ssh $NODE "mkdir -p $DM/ "
rsync -avz . $NODE:$DM

