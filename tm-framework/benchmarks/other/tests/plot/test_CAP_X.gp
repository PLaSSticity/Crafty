set term postscript color eps enhanced 22 size 15cm,7cm
# ARG1 - ACCOUNTS
# ARG2 - PATH_TO_DATA
# ARG3 - THREADS
set output sprintf("|ps2pdf -dEPSCrop - test_CAP_X_thr%s.pdf", ARG3)

#set grid ytics
#set grid xtics

set logscale x 2
set logscale y 10
set format y "10^{%T}"
set ytics offset 0.2,0 font ",30"
set mxtics
set xtics rotate by -45 left font ",30" offset -0.8,0.3

set lmargin 7.5
set rmargin 3.3
set tmargin 1
set bmargin 5

### TODO:

set key inside left bottom font ",34"
set ylabel "Throughput (TXs/s)" font ",34" offset -1.4,-0.0
set xlabel "Number of Cache Lines" font ",34" offset -0.0,-0.5
#set title sprintf("bank -u 100 -a %s -n %s -s", ARG1, ARG3)

HTM=sprintf("%s/test_HTM_thr%s_CAP.txt", ARG2, ARG3)
PHTM=sprintf("%s/test_PHTM_thr%s_CAP.txt", ARG2, ARG3)
NVHTM=sprintf("%s/test_NVHTM_thr%s_CAP.txt", ARG2, ARG3)

plot \
  HTM   u 1:7:($7-$14):($7+$14) title " HTM" with yerrorbars lc 1 pt 1 ps 3 lw 6, \
  HTM   u 1:7                 notitle with lines lc 1 lw 4, \
  NVHTM u 1:7:($7-$14):($7+$14) title 'NV-HTM' with yerrorbars lc 3 pt 8 ps 3 lw 6, \
  NVHTM u 1:7                 notitle with lines lc 3 lw 4, \
  PHTM  u 1:7:($7-$14):($7+$14) title 'PHTM' with yerrorbars lc 2 pt 6 ps 3 lw 6, \
  PHTM  u 1:7                 notitle with lines lc 2 lw 4, \
