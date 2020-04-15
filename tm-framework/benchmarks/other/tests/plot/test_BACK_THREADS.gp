set term postscript color eps enhanced 22
# ARG1 - PATH_TO_DATA
# ARG2 - ACCOUNTS
# ARG3 - THREADS
set output sprintf("|ps2pdf -dEPSCrop - test_BACKWARD_X_THREADS.pdf")

set grid ytics
set grid xtics

#set logscale x 2
set mxtics

### TODO:

set key outside right Left reverse
set ylabel "Throughput"
set xlabel "Update Rate"
set title sprintf('bank (Nb. accounts=%s, Nb. threads=%s)', ARG2, ARG3)

NVHTM1=sprintf("%s/test_NVHTM0_01_BACK.txt", ARG1)
NVHTM2=sprintf("%s/test_NVHTM0_05_BACK.txt", ARG1)
NVHTM3=sprintf("%s/test_NVHTM0_1_BACK.txt", ARG1)
NVHTM4=sprintf("%s/test_NVHTM0_5_BACK.txt", ARG1)
NVHTM_F=sprintf("%s/test_NVHTM_F_BACK.txt", ARG1)
NVHTM_W=sprintf("%s/test_NVHTM_W_BACK.txt", ARG1)
PHTM=sprintf("%s/test_PHTM_BACK.txt", ARG1)
HTM=sprintf("%s/test_HTM_BACK.txt", ARG1)

plot \
  NVHTM1 using ($1-0.1):6:($6-$13):($6+$13) title "NVHTM_B 0.01" with yerrorbars lc 1, \
  NVHTM1    using 1:6                 notitle with lines lc 1, \
  NVHTM2    using ($1-0.75):6:($6-$13):($6+$13) title 'NVHTM_B 0.05' with yerrorbars lc 2, \
  NVHTM2    using 1:6                 notitle with lines lc 2, \
  NVHTM3    using ($1-0.5):6:($6-$13):($6+$13) title 'NVHTM_B 0.1' with yerrorbars lc 3, \
  NVHTM3    using 1:6                 notitle with lines lc 3, \
  NVHTM4    using ($1-0.25):6:($6-$13):($6+$13) title 'NVHTM_B 0.5' with yerrorbars lc 4, \
  NVHTM4    using 1:6                 notitle with lines lc 4, \
  NVHTM_F   using ($1-0.0):6:($6-$13):($6+$13) title 'NVHTM_F' with yerrorbars lc 5, \
  NVHTM_F   using 1:6                 notitle with lines lc 5, \
  NVHTM_W   using ($1+0.25):6:($6-$13):($6+$13) title 'NVHTM_W' with yerrorbars lc 6, \
  NVHTM_W   using 1:6                 notitle with lines lc 6, \
  PHTM      using ($1+0.5):6:($6-$13):($6+$13) title 'PHTM' with yerrorbars lc 7, \
  PHTM      using 1:6                 notitle with lines lc 7, \
  HTM      using ($1+0.75):6:($6-$13):($6+$13) title 'HTM' with yerrorbars lc 8, \
  HTM      using 1:6                 notitle with lines lc 8, \
  