set term postscript color eps enhanced 22
# ARG1 - NAME PDF
# ARG2 - PATH_TO_DATA
# ARG3 - ACCOUNTS
# ARG4 - TX_SIZE
set output sprintf("|ps2pdf -dEPSCrop - test_%s_FLUSHES.pdf", ARG1)

set grid ytics
set grid xtics

set logscale y 2
set ytics 0,2,1e12

set format y "2^{%L}"

set xrange [-1:101]

### TODO:

set key outside right Left reverse
set ylabel "Nb. Flushes / Nb. TXs"
set xlabel "Update Rate (%)"
set title sprintf('bank (Nb. accounts=%s, TX\_SIZE=%s, Nb. threads=%s)', \
    ARG3, ARG4, ARG5)

NVHTM_B_10000  =sprintf("%s/test_NVHTM_B_10000.txt",   ARG2)
NVHTM_B_100000 =sprintf("%s/test_NVHTM_B_100000.txt",  ARG2)
NVHTM_B_1000000=sprintf("%s/test_NVHTM_B_1000000.txt", ARG2)
NVHTM_B_50_10000  =sprintf("%s/test_NVHTM_B_50_10000.txt",   ARG2)
NVHTM_B_50_100000 =sprintf("%s/test_NVHTM_B_50_100000.txt",  ARG2)
NVHTM_B_50_1000000=sprintf("%s/test_NVHTM_B_50_1000000.txt", ARG2)
NVHTM_F_10000  =sprintf("%s/test_NVHTM_F_10000.txt",   ARG2)
NVHTM_F_100000 =sprintf("%s/test_NVHTM_F_100000.txt",  ARG2)
NVHTM_F_1000000=sprintf("%s/test_NVHTM_F_1000000.txt", ARG2)

plot \
  NVHTM_B_10000    u ($1-0.75):9:($9-$19):($9+$19) notitle w yerrorbars lc 1 pt 4, \
  NVHTM_B_10000    u ($1-0.75):9                   title "B 99% (10k)" w  linespoints lc 1 pt 4, \
  NVHTM_B_100000   u ($1-0.5):9:($9-$19):($9+$19) notitle w yerrorbars lc 2 pt 6, \
  NVHTM_B_100000   u ($1-0.5):9                   title "B 99% (100k)" w linespoints lc 2 pt 4, \
  NVHTM_B_1000000  u ($1-0.25):9:($9-$19):($9+$19) notitle w yerrorbars lc 3 pt 8, \
  NVHTM_B_1000000  u ($1-0.25):9                   title "B 99% (1M)" w   linespoints lc 3 pt 8, \
  NVHTM_B_50_10000   u ($1-0):9:($9-$19):($9+$19) notitle w yerrorbars lc 1 pt 5, \
  NVHTM_B_50_10000   u ($1-0):9                   title "B 50% (10k)" w linespoints lc 1 dt "-" pt 5, \
  NVHTM_B_50_100000  u ($1+0.25):9:($9-$19):($9+$19) notitle w yerrorbars lc 2 pt 7, \
  NVHTM_B_50_100000  u ($1+0.25):9                   title "B 50% (100k)" w linespoints lc 2 dt "-" pt 7, \
  NVHTM_B_50_1000000 u ($1+0.5):9:($9-$19):($9+$19) notitle w yerrorbars lc 3 pt 9, \
  NVHTM_B_50_1000000 u ($1+0.5):9                   title "B 50% (1M)" w linespoints lc 3 dt "-" pt 9, \
  NVHTM_F_10000    u ($1+0.75):9:($9-$19):($9+$19) notitle w yerrorbars lc 1 pt 1, \
  NVHTM_F_10000    u ($1+0.75):9                   title "F (10k)" w linespoints lc 1 dt "." pt 1, \
  NVHTM_F_100000   u ($1+1):9:($9-$19):($9+$19) notitle w yerrorbars lc 2 pt 2, \
  NVHTM_F_100000   u ($1+1):9                   title "F (100k)" w linespoints lc 2 dt "." pt 2, \
  NVHTM_F_1000000  u ($1+1.25):9:($9-$19):($9+$19) notitle w yerrorbars lc 3 pt 3, \
  NVHTM_F_1000000  u ($1+1.25):9                   title "F (1M)" w linespoints lc 3 dt "." pt 3, \
