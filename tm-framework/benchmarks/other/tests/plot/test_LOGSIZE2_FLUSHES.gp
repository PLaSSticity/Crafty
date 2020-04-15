set term postscript color eps enhanced 22
# ARG1 - NAME PDF
# ARG2 - PATH_TO_DATA
# ARG3 - ACCOUNTS
# ARG4 - TX_SIZE
# ARG5 - UPDATE_RATE
# ARG6 - READ_SIZE
# ARG7 - NB_TXS
set output sprintf("|ps2pdf -dEPSCrop - test_%s_FLUSHES.pdf", ARG1)

set grid ytics
set grid xtics

set logscale y 2
set ytics 0,2,1e12 font ",10"

set format y "2^{%L}"
set mxtics

set xrange [0:29]

### TODO:

set key outside right Left reverse maxcols 1 maxrows 20 font ",16"
set ylabel "Nb. Flushes Checkpoint"
set xlabel "Number of Threads"
set title sprintf('bank (-a=%s, -s=%s, -u=%s, -r=%s, -d=%s)', \
    ARG3, ARG4, ARG5, ARG6, ARG7)

NVHTM_W     =sprintf("%s/test_NVHTM_W.txt",   ARG2)
PHTM        =sprintf("%s/test_PHTM.txt",   ARG2)
HTM         =sprintf("%s/test_HTM.txt",   ARG2)
NVHTM_B_5000000    =sprintf("%s/test_NVHTM_B_5000000.txt",   ARG2)
NVHTM_B_50000000   =sprintf("%s/test_NVHTM_B_50000000.txt",  ARG2)
NVHTM_B_100000000  =sprintf("%s/test_NVHTM_B_100000000.txt", ARG2)
NVHTM_B_150000000  =sprintf("%s/test_NVHTM_B_150000000.txt", ARG2)
NVHTM_B_250000000  =sprintf("%s/test_NVHTM_B_250000000.txt", ARG2)
NVHTM_B_50_5000000     =sprintf("%s/test_NVHTM_B_50_5000000.txt",   ARG2)
NVHTM_B_50_50000000    =sprintf("%s/test_NVHTM_B_50_50000000.txt",  ARG2)
NVHTM_B_50_100000000   =sprintf("%s/test_NVHTM_B_50_100000000.txt", ARG2)
NVHTM_B_50_150000000   =sprintf("%s/test_NVHTM_B_50_150000000.txt", ARG2)
NVHTM_B_50_250000000  =sprintf("%s/test_NVHTM_B_50_250000000.txt", ARG2)
NVHTM_F_5000000     =sprintf("%s/test_NVHTM_F_5000000.txt",   ARG2)
NVHTM_F_50000000    =sprintf("%s/test_NVHTM_F_50000000.txt",  ARG2)
NVHTM_F_100000000   =sprintf("%s/test_NVHTM_F_100000000.txt", ARG2)
NVHTM_F_150000000   =sprintf("%s/test_NVHTM_F_150000000.txt", ARG2)
NVHTM_F_250000000   =sprintf("%s/test_NVHTM_F_250000000.txt", ARG2)

plot \
  NVHTM_B_50_5000000    u ($1-0.15):9:($9-$22):($9+$22)  notitle                     lc 1 pt 4 w yerrorbars, \
  NVHTM_B_50_5000000    u ($1-0.15):9                    title "B (5M)"   dt "-" lc 1 pt 4 w linespoints, \
  NVHTM_B_50_150000000  u ($1-0.10):9:($9-$22):($9+$22) notitle                     lc 4 pt 10 w yerrorbars, \
  NVHTM_B_50_150000000  u ($1-0.10):9                   title "B (150M)" dt "-" lc 4 pt 10 w linespoints, \
  NVHTM_B_50_250000000 u ($1-0.05):9:($9-$22):($9+$22)  notitle                     lc 5 pt 12 w yerrorbars, \
  NVHTM_B_50_250000000 u ($1-0.05):9                    title "B (250M)"   dt "-" lc 5 pt 12 w linespoints, \
  NVHTM_F_5000000    u ($1+0.05):9:($9-$22):($9+$22) notitle                 lc 1 pt 3 w yerrorbars, \
  NVHTM_F_5000000    u ($1+0.05):9                   title "F (5M)"          lc 1 pt 3 w linespoints, \
  NVHTM_F_150000000  u ($1+0.1):9:($9-$22):($9+$22) notitle                 lc 4 pt 9 w yerrorbars, \
  NVHTM_F_150000000  u ($1+0.1):9                   title "F (150M)"        lc 4 pt 9 w linespoints, \
  NVHTM_F_250000000 u ($1+0.15):9:($9-$22):($9+$22) notitle                 lc 5 pt 11 w yerrorbars, \
  NVHTM_F_250000000 u ($1+0.15):9                   title "F (250M)"          lc 5 pt 11 w linespoints, \
