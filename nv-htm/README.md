# NV-HTM: Hardware Transactional Memory meets Memory Persistency [1]

This project is a prototype of the system presented in [1]. Every experiment was
carried using the code contained in this repo and in its dependencies.

## Dependencies

This project depends on the following external libraries:

* pmem: https://github.com/pmem/pmdk.git
* tcmalloc

Given that pmem is too intrusive and introduces overheads that interfere with
the normal functioning of HTM, Persistent Memory is emulated using the
following library:

* minimal-nvm: https://bitbucket.org/daniel_castro1993/minimal_nvm.git

The implementation of the HTM with SGL fallback is found in DEPENDENCIES. All
internal and external dependencies must be found in the system, in order to
simplify the building phase, we assume this project will run in a remote server
(with HTM and NVM), and we provide scripts to copy the sources remotely.

The benchmark tests can be found in:

* https://bitbucket.org/daniel_castro1993/tm-framework.git

It contains our synthetic benchmark bank, the STAMP test suit [2] and TPC-C [3].

## Coping to remote servers

Within each project, we provide a script called `compile_in_node.sh`. This
script compresses and copies through scp to the target server (e.g.,
`compile_in_node.sh nodeX` will send to nodeX). You must configure the
credentials for the server using ssh in `~/.ssh/config`.

By default, the sources are put in `~/projs/`.

## Code tree

Common code among all solutions is found in:

* ./common

The implementation for Avni's PHTM [4] is in:

* ./phtm

NV-HTM is in:

* ./nvhtm_common - common code
* ./nvhtm_lc     - using logical clock
* ./nvhtm_pc     - using physical clock

The HTM+SGL only version is located in:

* ./htm_only

## Compiling

This is a Makefile project, so to compile it one must call `make`. Depending on
the flags passed to the Makefile, the different test solutions can be compiled,
i.e., PHTM, HTM-only, NV-HTM-logical-clock and NV-HTM-physical-clock.

### Compiling HTM-only

In order to compile PHTM run the following command:

`make SOLUTION=1`

### Compiling PHTM

In order to compile PHTM run the following command:

`make SOLUTION=2`

### Compiling NV-HTM-logical-clock

In order to compile PHTM run the following command:

`make SOLUTION=3 DO_CHECKPOINT=5 SORT_ALG=5`

### Compiling NV-HTM-physical-clock

In order to compile PHTM run the following command:

`make SOLUTION=4 DO_CHECKPOINT=5 SORT_ALG=5`

---

#### References

[1] Castro, D., Romano, P., Barreto, J. (2018). Hardware Transactional Memory
meets Memory Persistency. IPDPS'18.

[2] Cao Minh, C., Chung, J., Kozyrakis, C., Olukotun (2008). STAMP: Stanford
Transactional Applications for Multi-Processing. IISWC'08, 35–46.

[3] Kohler, W., Shah, A., Raab, F., "Overview of TPC Benchmark C: The
Order-Entry Benchmark" technical report, Transaction Processing Performance
Council, December 23, 1991.

[4] Avni, H., Levy, E., Avi, M. (2015). Hardware Transactions in
Nonvolatile Memory. DISC'15, 617–630.

---

#### Thanks
This work was supported by Portuguese funds through Fundação para a Ciência e
Tecnologia via projects UID/CEC/50021/2013 and PTDC/EEISCR/1743/2014.
