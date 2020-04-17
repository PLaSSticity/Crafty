#!/usr/bin/env bash

if [[ $# -eq 0 ]] ; then
    echo "Please pass the name of the experiment as the first argument"
    exit 1
fi

cd /
Crafty.py $@
pushd /results >/dev/null
cp /Crafty/misc/results.tex .
cp /Crafty/misc/acmart.cls .
rm results.pdf
latexmk -pdf results.tex
popd >/dev/null
