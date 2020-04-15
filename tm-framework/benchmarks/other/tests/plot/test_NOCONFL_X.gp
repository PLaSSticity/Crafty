set term postscript color eps enhanced 22 size 15cm,6.5cm
# ARG1 - NAME PDF
# ARG2 - PATH_TO_DATA
# ARG3 - ACCOUNTS
# ARG4 - TX_SIZE
set output sprintf("|ps2pdf -dEPSCrop - test_%s_X.pdf", ARG1)

#set grid ytics
#set grid xtics
set ytics offset 0.4,0 font ",30"

#set logscale x 2
set mxtics
set xtics rotate by 45 right font ",30"

set xrange [1:30]
set yrange [0:7]

set lmargin 5.8
set rmargin 0.8
set tmargin 0.8
set bmargin 3.6

### TODO:

set key at graph 0.01,0.98 left top reverse Left font ",34" spacing 1.5
set ylabel "Throughput ({/*0.8 x10}^6 TXs/s)" font ",28" offset 0.5,-0.0
set xlabel "Number of Threads" font ",34" offset 1.2,-0.0
#set title sprintf('bank (Nb. accounts=%s, TX\_SIZE=%s)', ARG3, ARG4)

HTM  =sprintf("%s/test_HTM_%s.txt", ARG2, ARG1)
NVHTM=sprintf("%s/test_NVHTM_%s.txt", ARG2, ARG1)
NVHTM_LC=sprintf("%s/test_NVHTM_LC_%s.txt", ARG2, ARG1)
PHTM =sprintf("%s/test_PHTM_%s.txt", ARG2, ARG1)
NVHTM_W=sprintf("%s/test_NVHTM_W_%s.txt", ARG2, ARG1)
NVHTM_LC_W=sprintf("%s/test_NVHTM_LC_W_%s.txt", ARG2, ARG1)
NVHTM_B=sprintf("%s/test_NVHTM_B_%s.txt", ARG2, ARG1)
NVHTM_LC_B=sprintf("%s/test_NVHTM_LC_B_%s.txt", ARG2, ARG1)

plot \
  NVHTM_W  using ($1+0.05):($7/1e6):($7/1e6-$14/1e6):($7/1e6+$14/1e6) notitle with yerrorbars lc 1 pt 6 ps 3 lw 4, \
  NVHTM_W  using ($1+0.05):($7/1e6)                 title 'NV-HTM^{PC}' with linespoints lc 1 pt 6 ps 3 lw 4, \
  NVHTM_LC_W  using ($1+0.15):($7/1e6):($7/1e6-$14/1e6):($7/1e6+$14/1e6) notitle with yerrorbars lc 3 pt 7 ps 2 lw 4, \
  NVHTM_LC_W  using ($1+0.15):($7/1e6)              title 'NV-HTM^{LC}' with linespoints lc 2 pt 7 ps 3  dt "-" lw 4, \
