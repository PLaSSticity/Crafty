#!/bin/sh
# bash doesn't play nice and ignores the quotes after -f, so had to put this in a separate file
time -f 'Average total memory used: %K KB\nMaximum resident set size: %M KB\nSystem CPU-seconds: %S\nUser CPU-seconds: %U\nReal time elapsed seconds: %e\n' "$@"