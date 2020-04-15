set term postscript color eps enhanced 22
# ARG1 - NAME PDF
# ARG2 - PATH_TO_DATA
# ARG3 - ACCOUNTS
# ARG4 - TX_SIZE
# ARG5 - READ_SIZE
# ARG6 - THREADS
set output sprintf("|ps2pdf -dEPSCrop - test_%s_X.pdf", ARG1)

set grid ytics
set grid xtics

#set logscale x 2
set mxtics

### TODO:

set key outside right
set ylabel "Throughput"
set xlabel "Number of Threads"
set title sprintf('bank (ACCOUNTS=%s, TX\_SIZE=%s, READ\_SIZE=%s, THREADS=%s)', \
    ARG3, ARG4, ARG5, ARG6) font ",16"

HTM  =sprintf("%s/test_HTM_%s.txt", ARG2, ARG1)
NVHTM=sprintf("%s/test_NVHTM_%s.txt", ARG2, ARG1)
NVHTM_LC=sprintf("%s/test_NVHTM_LC_%s.txt", ARG2, ARG1)
PHTM =sprintf("%s/test_PHTM_%s.txt", ARG2, ARG1)
NVHTM_W=sprintf("%s/test_NVHTM_W_%s.txt", ARG2, ARG1)
NVHTM_LC_W=sprintf("%s/test_NVHTM_LC_W_%s.txt", ARG2, ARG1)

plot \
  HTM    using 1:6:($6-$12):($6+$12) title "HTM" with yerrorbars lc 1, \
  HTM    using 1:6                 notitle with lines lc 1, \
  NVHTM  using 1:6:($6-$12):($6+$12) title 'NVHTM' with yerrorbars lc 3 pt 4, \
  NVHTM  using 1:6                 notitle with lines lc 3, \
  NVHTM_LC  using 1:6:($6-$12):($6+$12) title 'NVHTM (LC)' with yerrorbars lc 4 pt 5, \
  NVHTM_LC  using 1:6                 notitle with lines lc 4 dt "-", \
  PHTM   using 1:6:($6-$12):($6+$12) title 'PHTM' with yerrorbars lc 2 pt 2, \
  PHTM   using 1:6                 notitle with lines lc 2, \
  NVHTM_W  using 1:6:($6-$12):($6+$12) title 'NVHTM_W' with yerrorbars lc 5 pt 6, \
  NVHTM_W  using 1:6                 notitle with lines lc 5, \
  NVHTM_LC_W  using 1:6:($6-$12):($6+$12) title 'NVHTM_W (LC)' with yerrorbars lc 6 pt 7, \
  NVHTM_LC_W  using 1:6                 notitle with lines lc 6 dt "-", \
  