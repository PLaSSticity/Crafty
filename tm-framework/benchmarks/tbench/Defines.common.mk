# kaan: These options determine what the benchmark is compiled with
CC       := gcc
CFLAGS   := -std=c++11 -g -w -pthread -fpermissive # -DNDEBUG=1 -mcpu=power8 -mtune=power8
OPT      ?= -O0
CFLAGS   += $(OPT)
CFLAGS   += -flto
#CFLAGS   += -I $(LIB)
CPP      := g++
CPPFLAGS := $(CFLAGS)
LD       := g++ -g
LIBS     += -lpthread

# Remove these files when doing clean
OUTPUT +=

LIB := ../lib
