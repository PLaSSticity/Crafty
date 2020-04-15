STM := ~/projs/tinystm/include

CC       := gcc
CFLAGS   += -std=c++11 -g -w -pthread -fpermissive
CFLAGS   += -I $(STM)
CPP      := g++
CPPFLAGS := $(CFLAGS)
LD       := g++
LIBS     += -lpthread

LIB := ../lib
