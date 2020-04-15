#!/bin/bash

if [[ $# -lt 1 ]] ; then
	exit
fi

tr -d '\r' < "$1" > "$1"_1234tmp
mv "$1"_1234tmp "$1"
