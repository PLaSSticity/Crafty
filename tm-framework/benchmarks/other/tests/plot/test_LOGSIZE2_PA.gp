set term postscript color eps enhanced 22
# ARG1 - NAME PDF
# ARG2 - PATH_TO_DATA
# ARG3 - PATH_TO_PLOTS
# ARG4 - ACCOUNTS
# ARG5 - TX_SIZE
# ARG6 - UPDATE_RATE
# ARG7 - READ_SIZE
# ARG8 - NB_TXS
set output sprintf("|ps2pdf -dEPSCrop - test_%s_PA.pdf", ARG1)

set grid ytics
set grid xtics

#set logscale x 2
set mxtics

set xrange [0:29]

### TODO:

set key outside right Left reverse maxcols 1 maxrows 20 font ",16"
set ylabel "Probability Abort"
set xlabel "Number of Threads"
set title sprintf('bank (-a=%s, -s=%s, -u=%s, -r=%s, -d=%s)', \
    ARG4, ARG5, ARG6, ARG7, ARG8)

call sprintf("%s/test_LOGSIZE2_files.inc", ARG3) ARG2

plot \
  NVHTM_B_50_5000000    u ($1-0.3):2:($2-$14):($2+$14) notitle                     lc 1 pt 4 w yerrorbars, \
  NVHTM_B_50_5000000    u ($1-0.3):2                   title "B 50% (5M)"   dt "-" lc 1 pt 4 w linespoints, \
  NVHTM_B_50_150000000  u ($1-0.0):2:($2-$14):($2+$14) notitle                     lc 4 pt 10 w yerrorbars, \
  NVHTM_B_50_150000000  u ($1-0.0):2                   title "B 50% (150M)" dt "-" lc 4 pt 10 w linespoints, \
  NVHTM_B_50_250000000 u ($1+0.1):2:($2-$14):($2+$14) notitle                     lc 5 pt 12 w yerrorbars, \
  NVHTM_B_50_250000000 u ($1+0.1):2                   title "B 50% (250M)"   dt "-" lc 5 pt 12 w linespoints, \
  NVHTM_F_5000000    u ($1-0.3):2:($2-$14):($2+$14) notitle                 lc 1 pt 5 w yerrorbars, \
  NVHTM_F_5000000    u ($1-0.3):2                   title "F (5M)"          lc 1 pt 5 w linespoints, \
  NVHTM_F_150000000  u ($1-0.0):2:($2-$14):($2+$14) notitle                 lc 4 pt 11 w yerrorbars, \
  NVHTM_F_150000000  u ($1-0.0):2                   title "F (150M)"        lc 4 pt 11 w linespoints, \
  NVHTM_F_250000000 u ($1+0.1):2:($2-$14):($2+$14) notitle                 lc 5 pt 13 w yerrorbars, \
  NVHTM_F_250000000 u ($1+0.1):2                   title "F (250M)"          lc 5 pt 13 w linespoints, \
  NVHTM_W u ($1+0.1):2:($2-$10):($2+$10) notitle         lc 6 pt 1 w yerrorbars, \
  NVHTM_W u ($1+0.1):2                   title "NVHTM_W" lc 6 pt 1 w linespoints, \
  PHTM    u ($1+0.1):2:($2-$10):($2+$10) notitle         lc 7 pt 2 w yerrorbars, \
  PHTM    u ($1+0.1):2                   title "PHTM"    lc 7 pt 2 w linespoints, \
  HTM    u ($1+0.1):2:($2-$9):($2+$9) notitle          lc 8 pt 3 w yerrorbars, \
  HTM    u ($1+0.1):2                   title "HTM"     lc 8 pt 3 w linespoints, \
  #NVHTM_F_50000000   u ($1-0.2):2:($2-$14):($2+$14) notitle                 lc 2 pt 7 w yerrorbars, \
  #NVHTM_F_50000000   u ($1-0.2):2                   title "F (50M)"         lc 2 pt 7 w linespoints, \
  #NVHTM_F_100000000  u ($1-0.1):2:($2-$14):($2+$14) notitle                 lc 3 pt 9 w yerrorbars, \
  #NVHTM_F_100000000  u ($1-0.1):2                   title "F (100M)"        lc 3 pt 9 w linespoints, \
  #NVHTM_B_50_50000000    u ($1-0.2):2:($2-$14):($2+$14) notitle                     lc 2 pt 6 w yerrorbars, \
  #NVHTM_B_50_50000000    u ($1-0.2):2                   title "B 50% (50M)"  dt "-" lc 2 pt 6 w linespoints, \
  #NVHTM_B_50_100000000  u ($1-0.1):2:($2-$14):($2+$14) notitle                     lc 3 pt 8 w yerrorbars, \
  #NVHTM_B_50_100000000  u ($1-0.1):2                   title "B 50% (100M)" dt "-" lc 3 pt 8 w linespoints, \
  #NVHTM_B_5000000    u ($1-0.3):2:($2-$14):($2+$14) notitle                     lc 1 pt 4 w yerrorbars, \
  #NVHTM_B_5000000    u ($1-0.3):2                   title "B 99% (5M)"   dt "." lc 1 pt 4 w linespoints, \
  #NVHTM_B_50000000   u ($1-0.2):2:($2-$14):($2+$14) notitle                     lc 2 pt 6 w yerrorbars, \
  #NVHTM_B_50000000   u ($1-0.2):2                   title "B 99% (50M)"  dt "." lc 2 pt 6 w linespoints, \
  #NVHTM_B_100000000  u ($1-0.1):2:($2-$14):($2+$14) notitle                     lc 3 pt 8 w yerrorbars, \
  #NVHTM_B_100000000  u ($1-0.1):2                   title "B 99% (100M)" dt "." lc 3 pt 8 w linespoints, \
  #NVHTM_B_250000000 u ($1+0.1):2:($2-$14):($2+$14) notitle                     lc 5 pt 12 w yerrorbars, \
  #NVHTM_B_250000000 u ($1+0.1):2                   title "B 99% (150M)"   dt "." lc 5 pt 12 w linespoints, \
  #NVHTM_B_150000000  u ($1-0.0):2:($2-$14):($2+$14) notitle                     lc 4 pt 10 w yerrorbars, \
  #NVHTM_B_150000000  u ($1-0.0):2                   title "B 99% (250M)" dt "." lc 4 pt 10 w linespoints, \
