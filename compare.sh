#!/bin/bash
set -e
set -o errexit -o pipefail -o nounset

LOGDIR="$HOME/results"

OPTS=hb:p:t:r:s:dag:vm:on
LONGOPTS=help,backend:,program:,threads:,runs:,solutions:,debug,stats,budget:,verbose,timeout:,build-only,gdb,massif,sde,show-output,experiment-name:,retry-no-delay,no-skip,nvm-latency:,log-size:,no-rollover-bits,nvm-latency-rdtscp

AVAILABLE_BACKENDS="stm-tinystm|htm-sgl-nvm"
DEFAULT_BACKEND="htm-sgl-nvm"

STAMP_PROGRAMS="bayes|genome|intruder|kmeans-high|kmeans-low|labyrinth|ssca2|vacation-high|vacation-low|yada"
OTHER_PROGRAMS="bank|bank-fee|bank-fee-nc|bank-fee-hc|simple|memory"
TBENCH_PROGRAMS="bplustree-wronly|bplustree"
AVAILABLE_PROGRAMS="$STAMP_PROGRAMS|$OTHER_PROGRAMS|tpcc|$TBENCH_PROGRAMS"
DEFAULT_PROGRAMS=("bayes" "genome" "kmeans-high" "kmeans-low" "labyrinth" "ssca2" "vacation-high" "vacation-low" "bank-fee" "bank-fee-hc" "bank-fee-nc" "bplustree" "bplustree-wronly")

declare -A SOLUTIONS=( ["HTM-only"]=1 ["PHTM"]=2 ["DudeTM"]=3 ["NV-HTM"]=4 ["Crafty"]=5 ["Crafty-NoRedo"]=5 ["Crafty-NoValidate"]=5)
declare -A CHECKPOINT=(["HTM-only"]=0 ["PHTM"]=0 ["DudeTM"]=5 ["NV-HTM"]=5 ["Crafty"]=0 ["Crafty-NoRedo"]=0 ["Crafty-NoValidate"]=0)
DEFAULT_SOLUTIONS=(     "HTM-only"                "DudeTM"     "NV-HTM"     "Crafty"     "Crafty-NoRedo"     "Crafty-NoValidate")

DEFAULT_THREADS=(1 2 4 8 12 15 16)
RUNS=5
BUDGET=20
NVM_LATENCY=300
NVM_LATENCY_RDTSCP=0
LOG_SIZE=1000000
STATS=''
NDEBUG='1'
OPT='-O3'
VERBOSE=''
TIMEOUT=5m
BUILDONLY=0
GDB=0
MASSIF=0
SDE=0
SHOW_OUTPUT=0
NO_SKIP=1
RETRY_NO_DELAY=0
ROLLOVER_BITS=1
NVM_WAIT_CYCLES=

SCRIPT=$(realpath $0)
SCRIPTPATH=$(dirname $SCRIPT)
LOGPATH="$SCRIPTPATH"/"$LOGDIR"
cd "$SCRIPTPATH"

usage() {
  local backend_usage=$(echo $AVAILABLE_BACKENDS | sed 's/|/,/g')
  local program_usage=$(echo $AVAILABLE_PROGRAMS | sed 's/|/,/g')
  local solutions_available=$(echo "${!SOLUTIONS[@]}" | sed 's/ /,/g')
  cat <<EOF
Usage: $0 [OPTION]...

Options:
  -b, --backend <$backend_usage>
		The backend to use for transactional memory.
        Defaults to $DEFAULT_BACKEND.
  -p, --program <$program_usage>
		The test program[s] to run. Can be repeated to run multiple tests.
        Defaults to [ ${DEFAULT_PROGRAMS[@]} ].
  -t, --threads <count>
        The number of threads to use when running the programs. Must be a positive integer.
        Can be repeated to run the tests multiple times, with different thread counts.
        Defaults to [ ${DEFAULT_THREADS[@]} ].
  -r, --runs <count>
        Each benchmark will be repeated this many times under each configuration. Must be a positive integer.
        Defaults to 10.
  -s, --solutions <$solutions_available>
        The solution to run the programs with. Can be repeated to compare multiple solutions.
        Defaults to [ ${DEFAULT_SOLUTIONS[@]} ].
  -n, --retry-no-delay
        Disable the random delay that is applied before retrying a transaction that failed due to conflicts.
  --no-rollover-bits
        Disable the "rollover bits", which are used to distinguish old entries from new ones in the log.
  -d, --debug
        Enable debugging.
  -a, --stats
        Enable stats collection.
  -g, --budget <retries>
        The number of retries that should be performed for each transaction before switching to SGL.
        Must be a positive integer. Defaults to 20.
  --nvm-latency <latency>
        The amount of time in nanoseconds that we wait to emulate NVM flush latency. Defaults to 300.
  --nvm-latency-rdtscp
        Use rdtscp to time the emulated NVM flush latency. If this option is not set, wall clock is used instead.
  --log-size <size>
        The amount of space to reserve for logs per thread, in number of log elements. Defaults to 1,000,000.
  -v, --verbose
        Enable verbose output, printing debug info during the entire run. Not recommended outside microbenchmarks.
  -m, --timeout
        Add a timeout, stopping the run after this long time. Enter as an integer, followed by s, m or h.
        For example, 2m for 2 minutes. Defaults to 5 minutes. Set to 0 to disable timeout.
  -o, --buildonly
        Only build the specified benchmarks, don't run them.
  --no-skip
        Always run the specified benchmark, even if there is an existing successful trial.
  --show-output
        Print the output to the screen, instead of printing it to a log file.
  --gdb
        Run the benchmark in gdb to debug it. It is recommended that you enable debugging along with this option.
  --massif
	Run the benchmark in valgrind, using massif to profile heap memory usage.
  --sde
        Run the benchmark in Intel Software Development Emulator to see detailed TSX stats.
EOF
}

nvm_latency_timing() { # Figure out how many cycles to wait to emulate NVM latency
    pushd misc
    echo "Estimating correct cycle count to emulate NVM lateny"
    gcc -O3 -o timing -DNVM_LATENCY_NS=${NVM_LATENCY} timing.c
    NVM_WAIT_CYCLES=$(./timing)
    echo "NVM latency wait is estimated as ${NVM_WAIT_CYCLES} cycles for ${NVM_LATENCY} nanoseconds."
    popd
}


build_nvm() { # Build NVM. Daniel suggested building NVM using the same parameters as the benchmarks.
    local NAME="$3"
    local RUN="$4"
    pushd nv-htm >/dev/null
    echo "Building NVM $NAME"
    local CRAFTY_REDO=1
    local CRAFTY_VALIDATE=1
    if [[ "$NAME" = "Crafty-NoRedo" ]] ; then CRAFTY_REDO=0 ; fi
    if [[ "$NAME" = "Crafty-NoValidate" ]] ; then CRAFTY_VALIDATE=0 ; fi
    make clean >& "$LOGPATH"/"$NAME.nvm.build.$RUN.log"
    make SOLUTION=$1 DO_CHECKPOINT=$2 VERBOSE=${VERBOSE} LOG_SIZE=${LOG_SIZE} SORT_ALG=5 FILTER=0.50 FILTER=0.1 CACHE_ALIGN_POOL=1 NDEBUG=${NDEBUG} NVM_LATENCY=${NVM_LATENCY} NVM_LATENCY_RDTSCP=${NVM_LATENCY_RDTSCP} OPT=${OPT} CRAFTY_STATS=${STATS} CRAFTY_REDO=${CRAFTY_REDO} CRAFTY_VALIDATE=${CRAFTY_VALIDATE} ROLLOVER_BITS=${ROLLOVER_BITS} BUDGET=${BUDGET} NVM_WAIT_CYCLES=${NVM_WAIT_CYCLES} RETRY_NO_DELAY=${RETRY_NO_DELAY} >> "$LOGPATH"/"$NAME.nvm.build.$RUN.log" 2>&1
    popd >/dev/null
}


build_libs() { # Run compile-all. Needs to be done once at the beginning, in case any code under nvm_emulation changed.
    echo "Building libs"
    NDEBUG=${NDEBUG} VERBOSE=${VERBOSE} BUDGET=${BUDGET} NVM_WAIT_CYCLES=${NVM_WAIT_CYCLES} NVM_LATENCY=${NVM_LATENCY} NVM_LATENCY_RDTSCP=${NVM_LATENCY_RDTSCP} \
        ./compile-all.sh > "$LOGPATH"/"libs.build.log" 2>&1
}


build_tests() {
    local NAME="$3"
    local BENCHDIR="$4"
    local RUN="$5"
    pushd "tm-framework/benchmarks/$BENCHDIR" > /dev/null
    echo "Building $NAME $BENCHDIR"
    local CRAFTY_REDO=1
    local CRAFTY_VALIDATE=1
    if [[ "$NAME" = "Crafty-NoRedo" ]] ; then CRAFTY_REDO=0 ; fi
    if [[ "$NAME" = "Crafty-NoValidate" ]] ; then CRAFTY_VALIDATE=0 ; fi
    MAKEFILE_ARGS="SOLUTION=$1 DO_CHECKPOINT=$2 LOG_SIZE=${LOG_SIZE} SORT_ALG=5
        FILTER=0.50 FILTER=0.1 CACHE_ALIGN_POOL=1 NDEBUG=$NDEBUG OPT=$OPT CRAFTY_STATS=$STATS CRAFTY_REDO=$CRAFTY_REDO CRAFTY_VALIDATE=$CRAFTY_VALIDATE NVM_LATENCY=${NVM_LATENCY} NVM_LATENCY_RDTSCP=${NVM_LATENCY_RDTSCP} ROLLOVER_BITS=${ROLLOVER_BITS} RETRY_NO_DELAY=${RETRY_NO_DELAY} VERBOSE=$VERBOSE BUDGET=$BUDGET" NVM_WAIT_CYCLES=${NVM_WAIT_CYCLES} \
        ./build.sh "$BACKEND" >& "$LOGPATH"/"$NAME.$BENCHDIR.build.$RUN.log"
    popd >/dev/null
}


# Make sure any other experiments left running are terminated
kill_all() {
    pkill bayes || true
    pkill genome || true
    pkill intruder || true
    pkill kmeans || true
    pkill labyrinth || true
    pkill ssca2 || true
    pkill vacation || true
    pkill yada || true
    pkill bank-fee || true
    pkill tpcc || true
    pkill bplustree || true
    pkill memcached || true
}


# libmemcached on most distributions doesn't include memaslap by default, so we have to build it ourselves.
# This source code in repository is libmemcached 1.0.18, patched to build in modern machines.
 build_memaslap() {
    pushd libmemcached >/dev/null
    rm -rf autom4te.cache config.status libtool mem_config.h stamp-h1
    ./configure --enable-memaslap
    sed -i 's_LDFLAGS =_LDFLAGS = -L/lib64 -lpthread_g' Makefile
    make
    popd >/dev/null
}


run_memaslap() {
    local THREADS="$2"
    local NAME="$3"
    local BENCHNAME="$4"
    local RUN="$5"
    # https://unix.stackexchange.com/questions/55913/whats-the-easiest-way-to-find-an-unused-local-port#comment271991_132524
    local port=$(python -c 'import socket; s=socket.socket(); s.bind(("", 0)); print(s.getsockname()[1]); s.close()')
    # Run the server (the PM application), in a subshell to avoid termination messages
    echo ./memcached -p "${port}" -t "${THREADS}"
    (./memcached -p "${port}" -t "${THREADS}" > "$LOGPATH"/"$NAME.$BENCHNAME-server.$THREADS""thr.run$RUN.log" 2>&1 &)
    # Run the client to generate the test load
    # TODO: Should we change the threads/concurrency here at all?
    sleep 2 # Wait for memcached to set everything up before we start memaslap
    echo memaslap --servers="localhost:${port}" --concurrency=4 --threads=4 --time=20s --binary
    ../../../../libmemcached/clients/memaslap --servers="localhost:${port}" --concurrency=4 --threads=4 --time=20s --binary > "${LOGPATH}/${NAME}.${BENCHNAME}.${THREADS}thr.run${RUN}.log" 2>&1
    pkill -SIGINT memcached || true
}


# Using the largest input sizes recommended in the STAMP paper
run_test() {
    local NAME="$3"
    local PROGRAM="$4"
    local BENCHDIR="$5"
    local THREADS="$6"
    local RUN="$7"
    echo "Running $PROGRAM on run $RUN with $NAME"
    local ARGS=""
    local BENCHNAME="$PROGRAM"
    local RUNDIR="$PROGRAM"
    local runner="timeout --foreground ${TIMEOUT} ${PWD}/time.sh"
    case "$PROGRAM" in
        "bayes")
            ARGS="-v32 -r4096 -n10 -p40 -i2 -e8 -s1 -t$THREADS"
            ;;
        "genome")
            ARGS="-g16384 -s64 -n16777216 -t$THREADS"
            ;;
        "intruder")
            ARGS="-a10 -l128 -n262144 -s1 -t$THREADS"
            ;;
        "kmeans-high")
            PROGRAM="kmeans"
            RUNDIR="kmeans"
            ARGS="-m15 -n15 -t0.00001 -i inputs/random-n65536-d32-c16.txt -p$THREADS"
            ;;
        "kmeans-low")
            PROGRAM="kmeans"
            RUNDIR="kmeans"
            ARGS="-m40 -n40 -t0.00001 -i inputs/random-n65536-d32-c16.txt -p$THREADS"
            ;;
        "labyrinth")
            ARGS="-i inputs/random-x512-y512-z7-n512.txt -t$THREADS"
            ;;
        "ssca2")
            ARGS="-s20 -i1.0 -u1.0 -l3 -p3 -t$THREADS"
            ;;
        "vacation-high")
            PROGRAM="vacation"
            RUNDIR="vacation"
            ARGS="-n4 -q60 -u90 -r1048576 -t4194304 -c$THREADS"
            ;;
        "vacation-low")
            PROGRAM="vacation"
            RUNDIR="vacation"
            ARGS="-n2 -q90 -u98 -r1048576 -t4194304 -c$THREADS"
            ;;
        "yada")
            ARGS="-a15 -i inputs/ttimeu1000000.2 -t$THREADS"
            ;;
        "bank" | "bank-fee")
            ARGS="$ARGS -n$THREADS -d4000000 -s5 -a4096 -b100000000"
            ;;
        "bank-fee-nc")
            ARGS="$ARGS -n$THREADS -c -d4000000 -s5 -a4096 -b100000000"
            PROGRAM="bank-fee"
            RUNDIR="bank-fee"
            ;;
        "bank-fee-hc")
            ARGS="$ARGS -n$THREADS -d4000000 -s5 -a1024 -b100000000"
            PROGRAM="bank-fee"
            RUNDIR="bank-fee"
            ;;
        "simple"|"memory")
            ARGS="$ARGS"
            ;;
        "bplustree")
            ARGS="$ARGS -t$THREADS -n50000000"
            RUNDIR="bplustree_mix"
            ;;
        "bplustree-wronly")
            ARGS="$ARGS -t$THREADS -n25000000"
            RUNDIR="bplustree"
            PROGRAM="bplustree"
            ;;
        "tpcc")
            ARGS="$ARGS -t 20 -m 30 -w 30 -s 0 -d 2 -o 26 -p 70 -r 2 -n $THREADS"
            #           -t 60 -m 30 -w 30 -s 0 -d 2 -o 26 -p 70 -r 2 -n $THREADS"
            # Above are the parameters from the file test_TPCC_c3.sh.
            # I think the following are supposed to be the high and low contention settings (found in file test_TPCC.sh)
            # HIGH      -t 5 -m 10 -w 10 -s 0 -d 0 -o 10 -p 90 -r 0 -n $THREADS
            # LOW       -t 5 -m 10 -w 10 -s 0 -d 0 -o 90 -p 10 -r 0 -n $THREADS
            RUNDIR="code"
            ;;
        "memcached")
            RUNDIR="output"
            runner="run_memaslap"
            ARGS="${THREADS} ${NAME} ${BENCHNAME} ${RUN}"
            ;;
    esac
    if [[ $GDB -eq 1 ]] ; then
        runner="gdb --args"
    elif [[ $MASSIF -eq 1 ]] ; then
        runner="valgrind --tool=massif"
    elif [[ $SDE -eq 1 ]] ; then
        runner="sde -tsx_stats_call_stack -tsx_stats -tsx --"
    fi
    kill_all
    ipcrm -M 0x00054321 1>/dev/null 2>&1 || true
    pushd "tm-framework/benchmarks/$BENCHDIR/$RUNDIR" > /dev/null
    if [[ ${SHOW_OUTPUT} -eq 1 ]] ; then
        ${runner} "./$PROGRAM" ${ARGS} || true
    else
        if (! grep -E "[Tt]ime *[0-9]+|Success|transactions in|Net_rate" "$LOGPATH"/"$NAME.$BENCHNAME.$THREADS""thr.run$RUN.log" 1>/dev/null 2>&1) || [[ ${NO_SKIP} -eq 1 ]] ; then
            if [[ "$PROGRAM" = "memcached" ]] ; then
                ${runner} "./$PROGRAM" ${ARGS} || true
                cat "$LOGPATH"/"$NAME.$BENCHNAME-server.$THREADS""thr.run$RUN.log" >> "$LOGPATH"/"$NAME.$BENCHNAME.$THREADS""thr.run$RUN.log"
            else
                echo "./$PROGRAM" $ARGS | tee "$LOGPATH"/"$NAME.$BENCHNAME.$THREADS""thr.run$RUN.log"
                ${runner} "./$PROGRAM" ${ARGS} >> "$LOGPATH"/"$NAME.$BENCHNAME.$THREADS""thr.run$RUN.log" 2>&1 || true
            fi
            grep -E "[Tt]ime *[0-9]+|Success|transactions in|Net_rate" "$LOGPATH"/"$NAME.$BENCHNAME.$THREADS""thr.run$RUN.log" || echo "!!!!!!!!!! FAILURE !!!!!!!!!!"
            # bank-fee doesn't calculate the expected amount correctly
#            if [[ "$PROGRAM" = "bank-fee" ]] ; then
#                local real expected
#                real=$(sed -nE 's_Bank amount after transactions: ([0-9]+) \(should be ([0-9]+)\)_\1_pg' "$LOGPATH"/"$NAME.$BENCHNAME.$THREADS""thr.run$RUN.log" 2>&1)
#                expected=$(sed -nE 's_Bank amount after transactions: ([0-9]+) \(should be ([0-9]+)\)_\2_pg' "$LOGPATH"/"$NAME.$BENCHNAME.$THREADS""thr.run$RUN.log" 2>&1)
#                if [[ "$real" != "$expected" ]] ; then
#                    echo "!!!!!!!!!! FAILURE !!!!!!!!!! expected ${expected}, found ${real}"
#                fi
#            fi
        else
            echo "Skipping ${BENCHNAME} with ${THREADS} thr on run ${RUN}"
        fi
    fi
    popd >/dev/null
}


! PARSED=$(getopt --options=$OPTS --longoptions=$LONGOPTS --name "$0" -- "$@")
if [[ ${PIPESTATUS[0]} -ne 0 ]]; then
    exit 1
fi
eval set -- "$PARSED"

mkdir -p "$LOGPATH"

PROGRAM_QUEUE=()
THREAD_QUEUE=()
SOLUTION_QUEUE=()
BACKEND="$DEFAULT_BACKEND"
HAS_STAMP=0
HAS_OTHER=0
HAS_TPCC=0
HAS_MEMCACHED=0
HAS_TBENCH=0
while [[ $# -gt 1 ]]; do
    case "$1" in
        -h|--help)
            usage
            exit 0
            ;;
        -b|--backend)
            match=$(echo "$2" | grep -Eo "$AVAILABLE_BACKENDS" || true)
            if [[ "$match" != "$2" ]] ; then
                echo "Unrecognized backend $2" >&2
                usage
                exit 2
            fi
            BACKEND=$2
            shift 2
            ;;
        -p|--program)
            match=$(echo "$2" | grep -Eo "$AVAILABLE_PROGRAMS" || true)
            if [[ "$match" != "$2" ]] ; then
                echo "Unrecognized program $2" >&2
                usage
                exit 3
            fi
            PROGRAM_QUEUE+=("$2")
            shift 2
            ;;
        -t|--threads)
            if [[ "$2" -lt 1 ]] ; then
                echo "Negative thread count $2 given" >&2
                usage
                exit 3
            fi
            THREAD_QUEUE+=("$2")
            shift 2
            ;;
        -r|--runs)
            if [[ "$2" -lt 1 ]] ; then
                echo "Runs cannot be less than 1" >&2
                usage
                exit 3
            fi
            RUNS="$2"
            shift 2
            ;;
        -s|--solutions)
            if [[ -z ${SOLUTIONS["$2"]} ]] ; then
                echo "Unrecognised solution $2" >&2
                usage
                exit 3
            fi
            SOLUTION_QUEUE+=("$2")
            shift 2
            ;;
        -a|--stats)
            STATS="1"
            shift 1
            ;;
        -d|--debug)
            NDEBUG=""
            OPT="-O0"
            shift 1
            ;;
        -g|--budget)
            if [[ "$2" -lt 0 ]] ; then
                echo "Negative budget $2 given" >&2
                usage
                exit 3
            fi
            BUDGET="$2"
            shift 2
            ;;
        --nvm-latency)
            if [[ "$2" -lt 0 ]] ; then
                echo "Negative NVM latency $2 given" >&2
                usage
                exit 3
            fi
            NVM_LATENCY="$2"
            shift 2
            ;;
        --nvm-latency-rdtscp)
            NVM_LATENCY_RDTSCP=1
            shift 1
            ;;
        --log-size)
            if [[ "$2" -lt 0 ]] ; then
                echo "Negative log size $2 given" >&2
                usage
                exit 3
            fi
            LOG_SIZE="$2"
            shift 2
            ;;
        -n|--retry-no-delay)
            RETRY_NO_DELAY="1"
            shift 1
            ;;
        -v|--verbose)
            VERBOSE="1"
            shift 1
            ;;
        -m|--timeout)
            TIMEOUT="$2"
            shift 2
            ;;
        -o|--build-only)
            BUILDONLY="1"
            shift 1
            ;;
        --no-skip)
            NO_SKIP="1"
            shift 1
            ;;
        --gdb)
            GDB="1"
            SHOW_OUTPUT="1"
            NO_SKIP="1"
            shift 1
            ;;
        --massif)
            MASSIF="1"
            SHOW_OUTPUT="1"
            NO_SKIP="1"
            shift 1
            ;;
        --sde)
            SDE="1"
            SHOW_OUTPUT="1"
            NO_SKIP="1"
            shift 1
            ;;
        --show-output)
            SHOW_OUTPUT="1"
            NO_SKIP="1"
            shift 1
            ;;
        --no-rollover-bits)
            ROLLOVER_BITS=0
            shift 1
            ;;
        --experiment-name)
            LOGPATH="${LOGPATH}/$2"
            if ! mkdir -p "$LOGPATH" ; then
                echo "Invalid experiment name, or file permission error when creating ${LOGPATH}}"
                exit 6
            fi
            if [[ -z $2 ]] ; then
                NO_SKIP="0"
            fi
            shift 2
            ;;
        *)
            echo "Unknown parameter $1"
            exit 5
            ;;
    esac
done

if [[ ${#PROGRAM_QUEUE[@]} -eq 0 ]] ; then
    PROGRAM_QUEUE=("${DEFAULT_PROGRAMS[@]}")
fi
if [[ ${#THREAD_QUEUE[@]} -eq 0 ]] ; then
    THREAD_QUEUE=("${DEFAULT_THREADS[@]}")
fi
if [[ ${#SOLUTION_QUEUE[@]} -eq 0 ]] ; then
    SOLUTION_QUEUE=("${DEFAULT_SOLUTIONS[@]}")
fi

echo -n "Running [ ${PROGRAM_QUEUE[*]} ] with thread counts [ ${THREAD_QUEUE[*]} ] using solutions [ ${SOLUTION_QUEUE[*]} ] backend [ $BACKEND ] for $RUNS runs with budget [ $BUDGET ]"
if [[ -n ${STATS} ]] ; then
    echo -n " <STATS> "
fi
if [[ -z ${NDEBUG} ]] ; then
    echo -n " <DEBUG> "
fi
if [[ -n ${VERBOSE} ]] ; then
    echo -n " <VERBOSE> "
fi
echo

if [[ -n ${NDEBUG} ]] ; then
    if [[ ${HAS_TPCC} -eq 1 ]] ; then
        echo "Can not run TPCC without debugging on, turn on debugging by adding the --debug flag." >&2
        exit 4
    fi
fi

if [[ ${NVM_LATENCY_RDTSCP} -eq 1 ]] ; then
    nvm_latency_timing
fi
build_libs
lastsolution=""
for program in "${PROGRAM_QUEUE[@]}" ; do
    match_stamp=$(echo "$program" | grep -Eo "$STAMP_PROGRAMS" || true)
    match_other=$(echo "$program" | grep -Eo "$OTHER_PROGRAMS" || true)
    match_tbench=$(echo "$program" | grep -Eo "$TBENCH_PROGRAMS" || true)
    if [[ "$match_stamp" == "$program" ]] ; then
        HAS_STAMP=1
    elif [[ "$match_other" == "$program" ]] ; then
        HAS_OTHER=1
    elif [[ "tpcc" == "$program" ]] ; then
        HAS_TPCC=1
    elif [[ "memcached" == "$program" ]] ; then
        HAS_MEMCACHED=1
        if [[ ! -f 'libmemcached/clients/memaslap' ]] ; then
            echo "Building memaslap"
            build_memaslap > "${LOGPATH}/memaslap.build.log" 2>&1
        fi
    elif [[ "$match_tbench" == "$program" ]] ; then
        HAS_TBENCH=1
    fi
done
for run in $(seq $RUNS) ; do
    for solution in "${SOLUTION_QUEUE[@]}" ; do
        if [[ "${lastsolution}" != "${solution}" ]] ; then
            build_nvm ${SOLUTIONS[$solution]} ${CHECKPOINT[$solution]} "$solution" "$run"
            if [[ ${HAS_OTHER} -eq 1 ]] ; then
                build_tests ${SOLUTIONS[$solution]} ${CHECKPOINT[$solution]} "$solution" "other" "$run"
            fi
            if [[ ${HAS_TPCC} -eq 1 ]] ; then
                build_tests ${SOLUTIONS[$solution]} ${CHECKPOINT[$solution]} "$solution" "tpcc" "$run"
            fi
            if [[ ${HAS_STAMP} -eq 1 ]] ; then
                build_tests ${SOLUTIONS[$solution]} ${CHECKPOINT[$solution]} "$solution" "stamp" "$run"
            fi
            if [[ ${HAS_TBENCH} -eq 1 ]] ; then
                build_tests ${SOLUTIONS[$solution]} ${CHECKPOINT[$solution]} "$solution" "tbench" "$run"
            fi
            if [[ ${HAS_MEMCACHED} -eq 1 ]] ; then
                build_tests ${SOLUTIONS[$solution]} ${CHECKPOINT[$solution]} "$solution" "memcached" "$run"
            fi
            if [[ ${BUILDONLY} -eq 1 ]] ; then continue; fi
        fi
        lastsolution="${solution}"
        for program in "${PROGRAM_QUEUE[@]}" ; do
            match_stamp=$(echo "$program" | grep -Eo "$STAMP_PROGRAMS" || true)
            match_other=$(echo "$program" | grep -Eo "$OTHER_PROGRAMS" || true)
            match_tbench=$(echo "$program" | grep -Eo "$TBENCH_PROGRAMS" || true)
            benchdir=""
            if [[ "$match_stamp" == "$program" ]] ; then
                benchdir="stamp"
            elif [[ "$match_other" == "$program" ]] ; then
                benchdir="other"
            elif [[ "tpcc" == "$program" ]] ; then
                benchdir="tpcc"
            elif [[ "memcached" == "$program" ]] ; then
                benchdir="memcached"
            elif [[ "$match_tbench" == "$program" ]] ; then
                benchdir="tbench"
            fi
            for thrcount in "${THREAD_QUEUE[@]}" ; do
                run_test ${SOLUTIONS[$solution]} ${CHECKPOINT[$solution]} "$solution" "$program" "$benchdir" "$thrcount" "$run"
            done
        done
    done
done
