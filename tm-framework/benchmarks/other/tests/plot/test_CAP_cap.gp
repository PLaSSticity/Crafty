set term postscript color eps enhanced 22
# ARG1 - ACCOUNTS
# ARG2 - PATH_TO_DATA
# ARG3 - THREADS
set output sprintf("|ps2pdf -dEPSCrop - test_CAP_Pcap_thr%s.pdf", ARG3)

set grid ytics
set grid xtics

set logscale x 2
set mxtics

### TODO:

set key outside right
set ylabel "Probability of Capacity Abort"
set xlabel "Number of written cache lines"
set title sprintf("bank (Nb. accounts=%s, THREADS=%s)", ARG1, ARG3)

HTM=sprintf("%s/test_HTM_thr%s_CAP.txt", ARG2, ARG3)
PHTM=sprintf("%s/test_PHTM_thr%s_CAP.txt", ARG2, ARG3)
NVHTM=sprintf("%s/test_NVHTM_thr%s_CAP.txt", ARG2, ARG3)

plot \
  HTM  using (2 * $1):4:($4-$10):($4+$10) title "HTM" with yerrorbars lc 1, \
  HTM     using (2 * $1):4                 notitle with lines lc 1, \
  PHTM    using (2 * $1):4:($4-$10):($4+$10) title 'PHTM' with yerrorbars lc 2, \
  PHTM    using (2 * $1):4                 notitle with lines lc 2, \
  NVHTM   using (2 * $1):4:($4-$10):($4+$10) title 'NVHTM' with yerrorbars lc 3, \
  NVHTM   using (2 * $1):4                 notitle with lines lc 3, \
