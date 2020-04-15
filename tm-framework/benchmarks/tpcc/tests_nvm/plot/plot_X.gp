set term postscript color eps enhanced 22 size 15cm,9cm
# ARG1 - PATH_TO_DATA
# ARG2 - BENCHMARK NAME
# ARG3 - BENCHMARK
set output sprintf("|ps2pdf -dEPSCrop - test_%s_X.pdf", ARG2)

set grid ytics
set grid xtics

set lmargin 7.5
set rmargin 9.3
set tmargin 1
set bmargin 4

#set logscale x 2
set mxtics
set xtics rotate by -45 left font ",30" offset -0.5,0.3
set ytics offset 0.4,0 font ",30"

### TODO:

set key outside top Left reverse maxcols 2 maxrows 3 font ",24" spacing 5
#set nokey
set ylabel "Throughput (x10^6TXs/s)" font ",34" offset 1.0,-0.0
set xlabel "Number of Threads" font ",34" offset -0.0,0.0
#set title font ",18" sprintf("%s", ARG3)

STM  =sprintf("%s/test_%s_STM.txt", ARG1, ARG2)
PSTM =sprintf("%s/test_%s_PSTM.txt", ARG1, ARG2)
HTM  =sprintf("%s/test_%s_HTM.txt", ARG1, ARG2)
PHTM =sprintf("%s/test_%s_PHTM.txt", ARG1, ARG2)
NVHTM_F_LC=sprintf("%s/test_%s_NVHTM_F_LC.txt", ARG1, ARG2)
NVHTM_B=sprintf("%s/test_%s_NVHTM_B.txt", ARG1, ARG2)
NVHTM_F=sprintf("%s/test_%s_NVHTM_F.txt", ARG1, ARG2)
NVHTM_W=sprintf("%s/test_%s_NVHTM_W.txt", ARG1, ARG2)

plot \
  STM     u ($1-0.05):($5/1e6):($5/1e6-$12/1e6):($5/1e6+$12/1e6) title "STM"  w yerrorbars lc 6 lw 4 ps 2, \
  STM     u ($1-0.05):($5/1e6)                                   notitle      w lines      lc 6 lw 4, \
  HTM     u ($1-0.05):($8/1e6):($8/1e6-$16/1e6):($8/1e6+$16/1e6) title "HTM"  w yerrorbars lc 1 lw 4 ps 2, \
  HTM     u ($1-0.05):($8/1e6)                                   notitle      w lines      lc 1 lw 4, \
  PSTM    u ($1-0.02):($5/1e6):($5/1e6-$12/1e6):($5/1e6+$12/1e6) title 'PSTM' w yerrorbars lc 7 lw 4 ps 2, \
  PSTM    u ($1-0.02):($5/1e6)                                   notitle      w lines      lc 7 lw 4, \
  PHTM    u ($1-0.02):($8/1e6):($8/1e6-$16/1e6):($8/1e6+$16/1e6) title 'PHTM' w yerrorbars lc 2 lw 4 ps 2, \
  PHTM    u ($1-0.02):($8/1e6)                                   notitle      w lines      lc 2 lw 4, \
  NVHTM_B u ($1+0.02):($8/1e6):($8/1e6-$16/1e6):($8/1e6+$16/1e6) title 'HTPM' w yerrorbars lc 4 lw 4 ps 2, \
  NVHTM_B u ($1+0.02):($8/1e6)                                   notitle      w lines      lc 4 lw 4, \
  #NVHTM_W u ($1+0.05):($8/1e6):($8/1e6-$16/1e6):($8/1e6+$16/1e6)  title 'HTPM_W'  w yerrorbars lc 5, \
  #NVHTM_W u 1:($8/1e6)                                         notitle         w lines      lc 5, \
