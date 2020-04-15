# Getting Started

Crafty requires HTM support, which is generally available in 8'th
generation or newer Intel processors as Intel TSX RTM. Some older
Intel processors where RTM support was disabled due to security
concerns may may be able to enable it by downgrading microcode.

The following instructions assumes that you are running a debian-based
(e.g. Ubuntu) machine with a bash shell. You can run it on other
setups, but may need to modify some commands.

First, you should check if RTM is supported on your machine. You can
check at `/proc/cpuinfo` for `rtm`, or check Intel's webpage for your
processor for "Intel® Transactional Synchronization Extensions".
Additionally, you can execute the following command (copy and paste
into bash). If the command runs cleanly, you have RTM support. If it
crashes due to an illegal instruction, you do not have RTM support.

```
echo '#include <immintrin.h>
      int main() {_xbegin();_xend();return 0;}' | gcc -mrtm -o rtm_test -x c - && ./rtm_test && echo 'RTM supported'
```

#### Building & running the docker image

Using the provided Docker container is not necessary but recommended.
If you would like to avoid using Docker, please ensure that you have
recent versions of the packages `g++ cmake libgoogle-perftools-dev`
installed, then skip this section. Note that the `Crafty` and
`results` folders must be in your home directory if you are not using
Docker. Otherwise, please continue following these instructions.

First, install and start docker.

```
sudo apt install docker.io
sudo systemctl start docker
```

Now, build the docker image.

```
sudo docker build --tag crafty:0.1 --tag crafty:latest .
```

Then, create a directory where the results of the experiments will be placed.
Whenever you run programs, you can find their results here.

```
cd
mkdir results
```

Finally, you can start up the docker image using the following
command.  Please ensure that the Crafty source code and the results
directories are both in the current directory (`$PWD`).

```
sudo docker run -v $PWD/results:/results:z -v $PWD/Crafty:/Crafty:z --rm -it crafty
```

This command will start up a docker container and place you inside it.


# Using the artifact

The docker image is set up so that any changes you make in the source
code directory will be reflected in the container. You can modify the
code in the `Crafty` directory to experiment with Crafty.

##### Code layout

Crafty's code inside the folder `Crafty` is organized as described in this
section.

* `htm-alg` contains the HTM algorithm. Especially of interest is
  `htm-alg/include/htm_retry_template.h`, which contains the abstract
  HTM algorithm.

* `tm-framework/benchmarks` contains the benchmark programs. Under this folder,
  `stamp` contains the stamp benchmarks, `other` contains the bank-fee
  benchmark as well as small examples `simple` and `memory`, and `tbench`
  contains the B+ tree benchmark.
  
* `nv-htm` contains the code for our and prior works methods. Under this
  folder, `crafty` contains the code for Crafty, `nvhtm_lc` contains the code
  for DudeTM, `nvhtm_pc` contains the code for NV-HTM, and `nvhtm_common`
  contains code shared between DudeTM and NV-HTM.
  
##### Running benchmarks & replicating paper results

The artifact contains a command named `compare.sh`, which will build and run
the project with whatever configuration you provide. This command must be run
in the docker container, i.e. the last command in section "running the image".

The command will place the results under the `results` folder. You can
access the results folder both through the container and through the directory
where you executed the `docker run` command.

The command has many options that can be adjusted. A full list of options can
be found by running `compare.sh --help`. The following examples will
demonstrate some of the useful options.

```
# Only run benchmarks bank-fee-hc (high conflict) and bank-fee-nc (no conflict)
compare.sh -p bank-fee-hc -p bank-fee-nc --experiment-name my-bank-fee-exp

# Run all benchmarks, but use only 1, 2 and 4 threads, print results to screen (instead of placing them under results)
compare.sh -t 1 -t 2 -t 4 --show-output

# Run all benchmarks, simulating NVM latency to be 100 nanoseconds
compare.sh --nvm-latency 100 --experiment-name fast-nvm-experiment

# Run all benchmarks, only using HTM-only and Crafty
compare.sh -s HTM-only -s Crafty --experiment-name my-crafty

# Run all benchmarks with all methods, doing only 1 run and enabling extra statistics for Crafty
compare.sh --stats -r 1 --experiment-name my-crafty-stats

# Run the genome benchmark with Crafty using 1 thread, under the GDB debugger
compare.sh -p genome -s Crafty -t 1 -r 1 --debug --gdb
```

If the execution of the command is interrupted, it can be resumed by simply
executing the same command again. If you would like to restart from scratch,
you may do so by deleting the `<experiment-name>` folder from results, or choosing
a different experiment name.

##### Replicating paper results

In order to recreate the exact results as the paper, you can execute the
following command. Note that the command will take around 8 hours to run. If
necessary, you can shorten this time by performing less runs, for example
adding the option `-r 3` will perform only 3 runs (instead of 5).

```
compare.sh --experiment-name my-experiment
```

Next, parse the results and replicate the paper's graphs using the following
command.

```
parse-results my-experiment
```

The results should now be available in PDF form, under the results folder.
