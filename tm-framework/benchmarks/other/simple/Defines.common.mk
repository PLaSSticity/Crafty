PROG := simple

SRCS += \
	simple.c \
	$(LIB)/mt19937ar.c \
	$(LIB)/random.c \
	$(LIB)/thread.c \

#
OBJS := $(addsuffix .o,$(basename $(SRCS)))

ifeq ($(CACHE_ALIGN_POOL),1)
EXTRA_DEFINES += -DCACHE_ALIGN_POOL=1
endif

ifeq ($(NDEBUG),1)
EXTRA_DEFINES += -DNDEBUG=1
endif

CFLAGS += $(EXTRA_DEFINES)
CPPFLAGS += $(EXTRA_DEFINES)
