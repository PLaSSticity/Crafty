set term postscript color eps enhanced 22
# ARG1 - PATH_TO_DATA
# ARG2 - ACCOUNTS
# ARG3 - THREADS
set output sprintf("|ps2pdf -dEPSCrop - test_BACKWARD_X_R%s.pdf", ARG4)

set grid ytics
set grid xtics

#set logscale x 2
set mxtics

### TODO:

set key outside right Left reverse
set ylabel "Throughput"
set xlabel "% Written Accounts"
set title sprintf('bank (Nb. accounts=%s, THREADS=%s, READ\_SIZE=%s)', \
    ARG2, ARG3, ARG4)

NVHTM1=sprintf("%s/test_NVHTM0_01_%s_BACK.txt", ARG1, ARG4)
NVHTM2=sprintf("%s/test_NVHTM0_05_%s_BACK.txt", ARG1, ARG4)
NVHTM3=sprintf("%s/test_NVHTM0_1_%s_BACK.txt", ARG1, ARG4)
NVHTM4=sprintf("%s/test_NVHTM0_5_%s_BACK.txt", ARG1, ARG4)
NVHTM_F=sprintf("%s/test_NVHTM_F_%s_BACK.txt", ARG1, ARG4)
NVHTM_W=sprintf("%s/test_NVHTM_W_%s_BACK.txt", ARG1, ARG4)
PHTM=sprintf("%s/test_PHTM_%s_BACK.txt", ARG1, ARG4)
HTM=sprintf("%s/test_HTM_%s_BACK.txt", ARG1, ARG4)

plot \
  NVHTM1 using 1:6:($6-$13):($6+$13) title "NVHTM_B 0.01" with yerrorbars lc 1, \
  NVHTM1    using 1:6                 notitle with lines lc 1, \
  NVHTM2    using 1:6:($6-$13):($6+$13) title 'NVHTM_B 0.05' with yerrorbars lc 2, \
  NVHTM2    using 1:6                 notitle with lines lc 2, \
  NVHTM3    using 1:6:($6-$13):($6+$13) title 'NVHTM_B 0.1' with yerrorbars lc 3, \
  NVHTM3    using 1:6                 notitle with lines lc 3, \
  NVHTM4    using 1:6:($6-$13):($6+$13) title 'NVHTM_B 0.5' with yerrorbars lc 4, \
  NVHTM4    using 1:6                 notitle with lines lc 4, \
  NVHTM_F   using 1:6:($6-$13):($6+$13) title 'NVHTM_F' with yerrorbars lc 5, \
  NVHTM_F   using 1:6                 notitle with lines lc 5, \
  NVHTM_W   using 1:6:($6-$13):($6+$13) title 'NVHTM_W' with yerrorbars lc 6, \
  NVHTM_W   using 1:6                 notitle with lines lc 6, \
  PHTM      using 1:6:($6-$13):($6+$13) title 'PHTM' with yerrorbars lc 7, \
  PHTM      using 1:6                 notitle with lines lc 7, \
  HTM      using 1:6:($6-$12):($6+$12) title  'HTM' with yerrorbars lc 8, \
  HTM      using 1:6                  notitle with lines lc 8, \
  