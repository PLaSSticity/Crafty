#!/bin/bash
set -e

echo Make sure the following packages are installed in case of build errors:
echo g++ cmake google-perftools libgoogle-perftools-dev

export NDEBUG VERBOSE BUDGET NVM_WAIT_CYCLES NVM_LATENCY_RDTSCP NVM_LATENCY

(cd htm-alg ; ./compile.sh)
(cd nvm-emulation ; ./compile.sh)
#(cd tinystm ; make)
