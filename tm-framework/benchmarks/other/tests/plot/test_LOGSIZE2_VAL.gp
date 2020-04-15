set term postscript color eps enhanced 22
# ARG1 - NAME PDF
# ARG2 - PATH_TO_DATA
# ARG4 - PATH_TO_PLOTS
# ARG5 - ACCOUNTS
# ARG6 - TX_SIZE
# ARG7 - UPDATE_RATE
# ARG8 - READ_SIZE
# ARG9 - NB_TXS
set output sprintf("|ps2pdf -dEPSCrop - test_%s_VAL.pdf", ARG1)

set grid ytics
set grid xtics

#set logscale x 2
set mxtics

set xrange [0:29]

### TODO:

set key outside right Left reverse maxcols 1 maxrows 20 font ",14" spacing 8
set ylabel "Ratio Time Waiting"
set xlabel "Number of Threads"
set title sprintf('bank (-a=%s, -s=%s, -u=%s, -r=%s, -d=%s)', \
    ARG4, ARG5, ARG6, ARG7, ARG8)

#NVHTM_W  =sprintf("%s/test_NVHTM_W.txt",  ARG1)
#PHTM     =sprintf("%s/test_PHTM.txt",     ARG1)
#HTM      =sprintf("%s/test_HTM.txt",      ARG1)
#STM      =sprintf("%s/test_STM.txt",      ARG1)
#PSTM     =sprintf("%s/test_PSTM.txt",     ARG1)
#NVHTM_B_25000000   =sprintf("%s/test_NVHTM_B_50_25000000.txt",   ARG1)
#NVHTM_B_50000000   =sprintf("%s/test_NVHTM_B_50_50000000.txt",   ARG1)
#NVHTM_B_100000000  =sprintf("%s/test_NVHTM_B_50_100000000.txt",  ARG1)
#NVHTM_B_250000000  =sprintf("%s/test_NVHTM_B_50_250000000.txt",  ARG1)
#NVHTM_B_N_25000000   =sprintf("%s/test_NVHTM_B_50_N_25000000.txt",   ARG1)
#NVHTM_B_N_100000000  =sprintf("%s/test_NVHTM_B_50_N_100000000.txt",  ARG1)
#NVHTM_B_N_250000000  =sprintf("%s/test_NVHTM_B_50_N_250000000.txt",  ARG1)
#NVHTM_F_1  =sprintf("%s/test_NVHTM_F_1.txt",     ARG1)
#NVHTM_F_2  =sprintf("%s/test_NVHTM_F_2.4.txt",   ARG1)
#NVHTM_F_8  =sprintf("%s/test_NVHTM_F_12.txt",    ARG1)
#NVHTM_B_1  =sprintf("%s/test_NVHTM_B_50_1.txt",   ARG1)
#NVHTM_B_2  =sprintf("%s/test_NVHTM_B_50_2.4.txt", ARG1)
#NVHTM_B_8  =sprintf("%s/test_NVHTM_B_50_12.txt",  ARG1)

REMOVE_LINES="<(sed '3d;6d;7d;9d;10d;11d'"

NVHTM_B_1=sprintf("%s %s/test_NVHTM_B_50_1.txt)",   REMOVE_LINES, ARG2)
NVHTM_B_2=sprintf("%s %s/test_NVHTM_B_50_2.4.txt)", REMOVE_LINES, ARG2)
NVHTM_B_8=sprintf("%s %s/test_NVHTM_B_50_12.txt)",  REMOVE_LINES, ARG2)
NVHTM_F_1=sprintf("%s %s/test_NVHTM_F_1.txt)",      REMOVE_LINES, ARG2)
NVHTM_F_2=sprintf("%s %s/test_NVHTM_F_2.4.txt)",    REMOVE_LINES, ARG2)
NVHTM_F_8=sprintf("%s %s/test_NVHTM_F_12.txt)",     REMOVE_LINES, ARG2)
NVHTM_W  =sprintf("%s %s/test_NVHTM_W.txt)",        REMOVE_LINES, ARG2)

NVHTM_B_1=sprintf("%s %s/test_NVHTM_B_50_1.txt)",   REMOVE_LINES, ARG2)
NVHTM_B_2=sprintf("%s %s/test_NVHTM_B_50_2.4.txt)", REMOVE_LINES, ARG2)
NVHTM_B_8=sprintf("%s %s/test_NVHTM_B_50_12.txt)",  REMOVE_LINES, ARG2)
NVHTM_F_1=sprintf("%s %s/test_NVHTM_F_1.txt)",      REMOVE_LINES, ARG2)
NVHTM_F_2=sprintf("%s %s/test_NVHTM_F_2.4.txt)",    REMOVE_LINES, ARG2)
NVHTM_F_8=sprintf("%s %s/test_NVHTM_F_12.txt)",     REMOVE_LINES, ARG2)
NVHTM_W  =sprintf("%s %s/test_NVHTM_W.txt)",        REMOVE_LINES, ARG2)

plot \
  NVHTM_W u ($1-0.00):10:($10-$20):($10+$20) notitle                 lc 2 pt 4  ps 2 lw 4 w yerrorbars, \
  NVHTM_W u ($1-0.00):10              title "HTPM_W"                 lc 2 pt 4  ps 2 lw 4 w linespoints, \
  NVHTM_B_1 u ($1-0.25):14:($14-$28):($14+$28) notitle lc 1 pt 4  ps 2 lw 4 w yerrorbars, \
  NVHTM_B_1 u ($1-0.25):14              title "HTPM\n(1 wrap)"       lc 1 pt 4  ps 2 lw 4 w linespoints, \
  NVHTM_B_2 u ($1-0.05):14:($14-$28):($14+$28) notitle lc 10 pt 8 ps 2 lw 4 w yerrorbars, \
  NVHTM_B_2 u ($1-0.05):14              title "HTPM\n(2.4 wraps)"    lc 10 pt 8 ps 2 lw 4 w linespoints, \
  NVHTM_B_8 u ($1-0.15):14:($14-$28):($14+$28) notitle lc 6 pt 6  ps 2 lw 4 w yerrorbars, \
  NVHTM_B_8 u ($1-0.15):14              title "HTPM\n(12 wraps)"     lc 6 pt 6  ps 2 lw 4 w linespoints, \
  NVHTM_F_1 u ($1+0.25):14:($14-$28):($14+$28) notitle               lc 1 pt 4  ps 2 lw 4 w yerrorbars, \
  NVHTM_F_1 u ($1+0.25):14    title "HTPM_F\n(1 wrap)" dt "."        lc 1 pt 4  ps 2 lw 4 w linespoints, \
  NVHTM_F_2 u ($1+0.05):14:($14-$28):($14+$28) notitle               lc 10 pt 8 ps 2 lw 4 w yerrorbars, \
  NVHTM_F_2 u ($1+0.05):14    title "HTPM_F\n(2.4 wraps)" dt "."     lc 10 pt 8 ps 2 lw 4 w linespoints, \
  NVHTM_F_8 u ($1+0.15):14:($14-$28):($14+$28) notitle               lc 6 pt 6  ps 2 lw 4 w yerrorbars, \
  NVHTM_F_8 u ($1+0.15):14    title "HTPM_F\n(12 wraps)" dt "."      lc 6 pt 6  ps 2 lw 4 w linespoints, \
#
