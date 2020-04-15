set term postscript color eps enhanced 22
# ARG1 - PATH_TO_DATA
# ARG2 - ACCOUNTS
# ARG3 - THREADS
set output sprintf("|ps2pdf -dEPSCrop - test_BACKWARD_PREC_BLOCKED_THREADS.pdf")

set grid ytics
set grid xtics

#set logscale x 2
set mxtics

### TODO:

set key outside right Left reverse
set ylabel "Ratio Time Blocked"
set xlabel "Number of Threads"
set title sprintf('bank (Nb. accounts=%s, UPDATE\_RATE=%s)', ARG2, ARG3)

NVHTM1=sprintf("%s/test_NVHTM0_01_BACK.txt", ARG1)
NVHTM2=sprintf("%s/test_NVHTM0_05_BACK.txt", ARG1)
NVHTM3=sprintf("%s/test_NVHTM0_1_BACK.txt", ARG1)
NVHTM4=sprintf("%s/test_NVHTM0_5_BACK.txt", ARG1)
NVHTM_F=sprintf("%s/test_NVHTM_F_BACK.txt", ARG1)
NVHTM_W=sprintf("%s/test_NVHTM_W_BACK.txt", ARG1)
PHTM=sprintf("%s/test_PHTM_BACK.txt", ARG1)

plot NVHTM1 using ($1+0.2*(rand(0)-0.5)):7:($7-$14):($7+$14) title "NVHTM_B 0.01" with yerrorbars lc 1, \
  NVHTM1    using 1:7                 notitle with lines lc 1, \
  NVHTM2    using ($1+0.2*(rand(0)-0.5)):7:($7-$14):($7+$14) title 'NVHTM_B 0.05' with yerrorbars lc 2, \
  NVHTM2    using 1:7                 notitle with lines lc 2, \
  NVHTM3    using ($1+0.2*(rand(0)-0.5)):7:($7-$14):($7+$14) title 'NVHTM_B 0.1' with yerrorbars lc 3, \
  NVHTM3    using 1:7                 notitle with lines lc 3, \
  NVHTM4    using ($1+0.2*(rand(0)-0.5)):7:($7-$14):($7+$14) title 'NVHTM_B 0.5' with yerrorbars lc 4, \
  NVHTM4    using 1:7                 notitle with lines lc 4, \
  NVHTM_F   using ($1+0.2*(rand(0)-0.5)):7:($7-$14):($7+$14) title 'NVHTM_F' with yerrorbars lc 5, \
  NVHTM_F   using 1:7                 notitle with lines lc 5, \
  NVHTM_W   using ($1+0.2*(rand(0)-0.5)):7:($7-$14):($7+$14) title 'NVHTM_W' with yerrorbars lc 6, \
  NVHTM_W   using 1:7                 notitle with lines lc 6, \
  PHTM      using ($1+0.2*(rand(0)-0.5)):7:($7-$14):($7+$14) title 'PHTM' with yerrorbars lc 7, \
  PHTM      using 1:7                 notitle with lines lc 7, \
  