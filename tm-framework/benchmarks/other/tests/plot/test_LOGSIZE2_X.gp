set term postscript color eps enhanced 22 size 15cm,15cm
# ARG1 - NAME PDF
# ARG2 - PATH_TO_DATA
# ARG3 - PATH_TO_PLOTS
# ARG4 - ACCOUNTS
# ARG5 - TX_SIZE
# ARG6 - UPDATE_RATE
# ARG7 - READ_SIZE
# ARG8 - NB_TXS
set output sprintf("|ps2pdf -dEPSCrop - test_%s_X.pdf", ARG1)

set grid ytics
set grid xtics

set lmargin 6.8
set rmargin 3.3
set tmargin 10
set bmargin 4

#set logscale y 10
set mxtics
#set format y "%.2tx10^{%T}"
set xtics rotate by -45 left font ",30" offset -0.8,0.3
set ytics offset 0.8,0 font ",30"

set xrange [0:29]

### TODO:

set key at graph 1.0,1.3 right Left reverse maxcols 2 maxrows 3 font ",12" spacing 4
#set nokey
set ylabel "Throughput (x10^6TXs/s)" font ",34" offset 0.8,-0.0
set xlabel "Number of Threads" font ",34" offset -0.0,0.0
#set title sprintf('bank (-a=%s, -s=%s, -u=%s, -r=%s, -d=%s)', \
#    ARG3, ARG4, ARG5, ARG6, ARG7)

call sprintf("%s/IMPORT_files.inc", ARG3) ARG2

plot \
  NVHTM_B_1 u ($1-0.25):($7/1e6):($7/1e6-$21/1e6):($7/1e6+$21/1e6) notitle lc 1 pt 4  ps 2 lw 4 w yerrorbars, \
  NVHTM_B_1 u ($1-0.25):($7/1e6)              title "HTPM\n(1 wrap)"       lc 1 pt 4  ps 2 lw 4 w linespoints, \
  NVHTM_B_2 u ($1-0.05):($7/1e6):($7/1e6-$21/1e6):($7/1e6+$21/1e6) notitle lc 10 pt 8 ps 2 lw 4 w yerrorbars, \
  NVHTM_B_2 u ($1-0.05):($7/1e6)              title "HTPM\n(2.4 wraps)"    lc 10 pt 8 ps 2 lw 4 w linespoints, \
  NVHTM_B_8 u ($1-0.15):($7/1e6):($7/1e6-$21/1e6):($7/1e6+$21/1e6) notitle lc 6 pt 6  ps 2 lw 4 w yerrorbars, \
  NVHTM_B_8 u ($1-0.15):($7/1e6)              title "HTPM\n(12 wraps)"     lc 6 pt 6  ps 2 lw 4 w linespoints, \
  NVHTM_F_1 u ($1-0.25):($7/1e6):($7/1e6-$21/1e6):($7/1e6+$21/1e6) notitle lc 1 pt 5  ps 1.6 lw 4 w yerrorbars, \
  NVHTM_F_1 u ($1-0.25):($7/1e6)       title "HTPM_F\n(1 wrap)" dt "."     lc 1 pt 5  ps 1.6 lw 4 w linespoints, \
  NVHTM_F_2 u ($1-0.05):($7/1e6):($7/1e6-$21/1e6):($7/1e6+$21/1e6) notitle lc 10 pt 9 ps 1.6 lw 4 w yerrorbars, \
  NVHTM_F_2 u ($1-0.05):($7/1e6)       title "HTPM_F\n(2.4 wraps)" dt "."  lc 10 pt 9 ps 1.6 lw 4 w linespoints, \
  NVHTM_F_8 u ($1-0.15):($7/1e6):($7/1e6-$21/1e6):($7/1e6+$21/1e6) notitle lc 6 pt 7  ps 1.6 lw 4 w yerrorbars, \
  NVHTM_F_8 u ($1-0.15):($7/1e6)       title "HTPM_F\n(12 wraps)" dt "."   lc 6 pt 7  ps 1.6 lw 4 w linespoints, \
  NVHTM_W   u ($1+0.05):($7/1e6):($7/1e6-$16/1e6):($7/1e6+$16/1e6) notitle lc 1 pt 1  ps 2 lw 4 w yerrorbars, \
  NVHTM_W   u ($1+0.05):($7/1e6)                   title "HTPM_W" dt "."   lc 1 pt 1  ps 2 lw 4 w linespoints, \
  PHTM      u ($1+0.15):($7/1e6):($7/1e6-$16/1e6):($7/1e6+$16/1e6) notitle lc 7 pt 2  ps 2 lw 4 w yerrorbars, \
  PHTM      u ($1+0.15):($7/1e6)                   title "PHTM"  dt "-.-"  lc 7 pt 2  ps 2 lw 4 w linespoints, \
  PSTM      u ($1+0.25):($3/1e6):($3/1e6-$8/1e6):($3/1e6+$8/1e6) notitle   lc 4 pt 11 ps 2 lw 4 w yerrorbars, \
  PSTM      u ($1+0.25):($3/1e6)                   title "PSTM"  dt "-.-"  lc 4 pt 11 ps 2 lw 4 w linespoints, \
#
