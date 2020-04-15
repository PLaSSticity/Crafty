set term postscript color eps enhanced 22
# ARG1 - NAME PDF
# ARG2 - PATH_TO_DATA
# ARG3 - ACCOUNTS
# ARG4 - TX_SIZE
set output sprintf("|ps2pdf -dEPSCrop - test_%s_BLOCK.pdf", ARG1)

set grid ytics
set grid xtics

#set logscale x 2
set mxtics

set xrange [1:24]

### TODO:

set key outside right
set ylabel "Ratio Time Blocked"
set xlabel "Number of Threads"
set title sprintf('bank (Nb. accounts=%s, TX\_SIZE=%s)', ARG3, ARG4)

HTM  =sprintf("%s/test_HTM_%s.txt", ARG2, ARG1)
NVHTM=sprintf("%s/test_NVHTM_%s.txt", ARG2, ARG1)
NVHTM_LC=sprintf("%s/test_NVHTM_LC_%s.txt", ARG2, ARG1)
PHTM =sprintf("%s/test_PHTM_%s.txt", ARG2, ARG1)
NVHTM_W=sprintf("%s/test_NVHTM_W_%s.txt", ARG2, ARG1)
NVHTM_LC_W=sprintf("%s/test_NVHTM_LC_W_%s.txt", ARG2, ARG1)
NVHTM_B=sprintf("%s/test_NVHTM_B_%s.txt", ARG2, ARG1)
NVHTM_LC_B=sprintf("%s/test_NVHTM_LC_B_%s.txt", ARG2, ARG1)

plot \
  NVHTM_W  using ($1+0.05):7:($7-$14):($7+$14) notitle with yerrorbars lc 5 pt 6, \
  NVHTM_W  using ($1+0.05):7                 title 'NVHTM_W' with linespoints lc 5 pt 6, \
  NVHTM_LC_W  using ($1+0.15):7:($7-$14):($7+$14) notitle with yerrorbars lc 5 pt 7, \
  NVHTM_LC_W  using ($1+0.15):7                 title 'NVHTM_W (LC)' with linespoints lc 5 dt "-" pt 7, \
