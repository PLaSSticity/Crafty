set term postscript color eps enhanced 22
# ARG1 - NAME PDF
# ARG2 - PATH_TO_DATA
# ARG3 - PATH_TO_PLOTS
# ARG4 - ACCOUNTS
# ARG5 - TX_SIZE
# ARG6 - UPDATE_RATE
# ARG7 - READ_SIZE
# ARG8 - NB_TXS
set output sprintf("|ps2pdf -dEPSCrop - test_%s_BLOCKS.pdf", ARG1)

set grid ytics
set grid xtics

#set logscale y 10
set mxtics

set xrange [0:29]

### TODO:

set key outside right Left reverse maxcols 1 maxrows 20 font ",14" spacing 8
set ylabel "Nb. Times Blocked"
set xlabel "Number of Threads"
set title sprintf('bank (-a=%s, -s=%s, -u=%s, -r=%s, -d=%s)', \
    ARG4, ARG5, ARG6, ARG7, ARG8)

call sprintf("%s/IMPORT_files.inc", ARG3) ARG2

plot \
  NVHTM_B_1 u ($1-0.25):13:($13-$27):($13+$27) notitle lc 1 pt 4  ps 2 lw 4 w yerrorbars, \
  NVHTM_B_1 u ($1-0.25):13              title "HTPM\n(1 wrap)"         lc 1 pt 4  ps 2 lw 4 w linespoints, \
  NVHTM_B_2 u ($1-0.05):13:($13-$27):($13+$27) notitle lc 10 pt 8 ps 2 lw 4 w yerrorbars, \
  NVHTM_B_2 u ($1-0.05):13              title "HTPM\n(2.4 wraps)"      lc 10 pt 8 ps 2 lw 4 w linespoints, \
  NVHTM_B_8 u ($1-0.15):13:($13-$27):($13+$27) notitle lc 6 pt 6  ps 2 lw 4 w yerrorbars, \
  NVHTM_B_8 u ($1-0.15):13              title "HTPM\n(12 wraps)"       lc 6 pt 6  ps 2 lw 4 w linespoints, \
  NVHTM_F_1 u ($1-0.25):13:($13-$27):($13+$27) notitle lc 1 pt 4  ps 2 lw 4 w yerrorbars, \
  NVHTM_F_1 u ($1-0.25):13          title "HTPM_F\n(1 wrap)" dt "."    lc 1 pt 4  ps 2 lw 4 w linespoints, \
  NVHTM_F_2 u ($1-0.05):13:($13-$27):($13+$27) notitle lc 10 pt 8 ps 2 lw 4 w yerrorbars, \
  NVHTM_F_2 u ($1-0.05):13          title "HTPM_F\n(2.4 wraps)" dt "." lc 10 pt 8 ps 2 lw 4 w linespoints, \
  NVHTM_F_8 u ($1-0.15):13:($13-$27):($13+$27) notitle lc 6 pt 6  ps 2 lw 4 w yerrorbars, \
  NVHTM_F_8 u ($1-0.15):13          title "HTPM_F\n(12 wraps)"  dt "." lc 6 pt 6  ps 2 lw 4 w linespoints, \
#
