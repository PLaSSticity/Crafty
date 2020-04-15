#!/bin/bash

g++ -o main.out main.c -L ../bin -l htm_sgl -I ../include -I \
	../../arch_dep/include -mrtm -g
