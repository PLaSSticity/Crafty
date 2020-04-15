set term postscript color eps enhanced 22 size 10cm,15cm
# ARG1 - ACCOUNTS
# ARG2 - PATH_TO_DATA
# ARG3 - THREADS
set output sprintf("|ps2pdf -dEPSCrop - test_CAP_PA_thr%s.pdf", ARG3)

set grid ytics
set grid xtics
set ytics offset 0.4,0 font ",30"

set yrange [0:1]

set logscale x 2
set mxtics
set xtics rotate by 45 right font ",30"

set lmargin 7
set rmargin 1
set tmargin 1
set bmargin 6

### TODO:

set key inside left top font ",34"
set ylabel "Probability of Abort" font ",34" offset 1.2,-0.0
set xlabel "Number of Cache Lines" font ",34" offset -0.0,-0.8
# set title sprintf("bank (Nb. accounts=%s, THREADS=%s)", ARG1, ARG3)

HTM=sprintf("%s/test_HTM_thr%s_CAP.txt", ARG2, ARG3)
PHTM=sprintf("%s/test_PHTM_thr%s_CAP.txt", ARG2, ARG3)
NVHTM=sprintf("%s/test_NVHTM_thr%s_CAP.txt", ARG2, ARG3)

plot \
  HTM  using (2 * $1):2:($2-$8):($2+$8) title " HTM" with yerrorbars lc 1 pt 1 ps 2 lw 4, \
  HTM     using (2 * $1):2                 notitle with lines lc 1 lw 3, \
  NVHTM   using (2 * $1):2:($2-$8):($2+$8) title ' HTPM' with yerrorbars lc 3 pt 8 ps 2 lw 4, \
  NVHTM   using (2 * $1):2                 notitle with lines lc 3 lw 3, \
  PHTM    using (2 * $1):2:($2-$8):($2+$8) title ' PHTM' with yerrorbars lc 2 pt 6 ps 2 lw 4, \
  PHTM    using (2 * $1):2                 notitle with lines lc 2 lw 3, \
