#!/bin/bash

ARCH_INC=../arch-dep/include

rm -r build
rm -r bin
mkdir build
cd build

export NVM_WAIT_CYCLES NDEBUG VERBOSE BUDGET NVM_LATENCY_RDTSCP NVM_LATENCY

# -DCMAKE_C_FLAGS_DEBUG="-g3 -gdwarf-2" -DCMAKE_CXX_FLAGS_DEBUG="-g3 -gdwarf-2" 
cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Prod \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON  -DARCH_INC_DIR=$ARCH_INC
make clean
make -j8
