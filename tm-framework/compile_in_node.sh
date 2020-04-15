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

ssh $NODE "mkdir -p $DM/ "
rsync -avz . $NODE:$DM

