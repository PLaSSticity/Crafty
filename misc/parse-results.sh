#!/usr/bin/env bash

if [[ $# -eq 0 ]] ; then
    echo "Please pass the name of the experiment as the first argument"
    exit 1
fi

pushd /results >/dev/null
Crafty.py $@
cp /Crafty/misc/results.tex .
cp /Crafty/misc/acmart.cls .
latexmk -pdf results.tex
popd >/dev/null
