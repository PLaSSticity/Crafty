set term postscript color eps enhanced 22 size 15cm,6.5cm
# ARG1 - PATH_TO_DATA
# ARG2 - BENCHMARK NAME
# ARG3 - BENCHMARK
set output sprintf("|ps2pdf -dEPSCrop - test_X_MULT.pdf", ARG2)

#set grid ytics
#set grid xtics

set lmargin 7.5
set rmargin 9.3
set tmargin 1
set bmargin 4

#set logscale x 2
set mxtics
set xtics 5,5,35 rotate by -45 left font ",30" offset -0.5,0.3
set ytics 0,1,10 offset 0.4,0 font ",30"

set yrange [0:3]
set xrange [0:29]

### TODO:

set key at graph 0.95,0.99 reverse Left maxcols 2 maxrows 2 font ",19" spacing 4
set ylabel "Throughput ({/*0.8 x10}^6 TXs/s)" font ",28" offset 1.0,-0.0
#set title font ",18" sprintf("%s", ARG3)

H_STM    =sprintf("%s/test_TPCC_H_STM.txt",     ARG1)
H_PSTM   =sprintf("%s/test_TPCC_H_PSTM.txt",    ARG1)
H_HTM    =sprintf("%s/test_TPCC_H_HTM.txt",     ARG1)
H_PHTM   =sprintf("%s/test_TPCC_H_PHTM.txt",    ARG1)
H_NVHTM_B=sprintf("%s/test_TPCC_H_NVHTM_B.txt", ARG1)

L_STM    =sprintf("%s/test_TPCC_L_STM.txt",     ARG1)
L_PSTM   =sprintf("%s/test_TPCC_L_PSTM.txt",    ARG1)
L_HTM    =sprintf("%s/test_TPCC_L_HTM.txt",     ARG1)
L_PHTM   =sprintf("%s/test_TPCC_L_PHTM.txt",    ARG1)
L_NVHTM_B=sprintf("%s/test_TPCC_L_NVHTM_B.txt", ARG1)

set multiplot layout 1, 2 margins 0.083,0.99,0.23,0.96 spacing 0.01

set xlabel "TPC-C (high)" font ",34" offset -0.0,0.0

plot \
  H_STM     u ($1-0.05):($5/1e6):($5/1e6-$12/1e6):($5/1e6+$12/1e6) title "STM"  w yerrorbars lc 6 lw 4 ps 2, \
  H_STM     u ($1-0.05):($5/1e6)                                   notitle      w lines      lc 6 lw 4, \
  H_HTM     u ($1-0.05):($8/1e6):($8/1e6-$16/1e6):($8/1e6+$16/1e6) title "HTM"  w yerrorbars lc 1 lw 4 ps 2, \
  H_HTM     u ($1-0.05):($8/1e6)                                   notitle      w lines      lc 1 lw 4, \
  H_PSTM    u ($1-0.02):($5/1e6):($5/1e6-$12/1e6):($5/1e6+$12/1e6) title 'PSTM' w yerrorbars lc 7 lw 4 ps 2, \
  H_PSTM    u ($1-0.02):($5/1e6)                                   notitle      w lines      lc 7 lw 4, \
  H_PHTM    u ($1-0.02):($8/1e6):($8/1e6-$16/1e6):($8/1e6+$16/1e6) title 'PHTM' w yerrorbars lc 2 lw 4 ps 2, \
  H_PHTM    u ($1-0.02):($8/1e6)                                   notitle      w lines      lc 2 lw 4, \
  H_NVHTM_B u ($1+0.02):($8/1e6):($8/1e6-$16/1e6):($8/1e6+$16/1e6) title 'HTPM' w yerrorbars lc 4 lw 4 ps 2, \
  H_NVHTM_B u ($1+0.02):($8/1e6)                                   notitle      w lines      lc 4 lw 4, \

unset key
unset ytics
unset ylabel

set xlabel "TPC-C (low)" font ",34" offset -0.0,0.0

plot \
  L_STM     u ($1-0.05):($5/1e6):($5/1e6-$12/1e6):($5/1e6+$12/1e6) title "STM"  w yerrorbars lc 6 lw 4 ps 2, \
  L_STM     u ($1-0.05):($5/1e6)                                   notitle      w lines      lc 6 lw 4, \
  L_HTM     u ($1-0.05):($8/1e6):($8/1e6-$16/1e6):($8/1e6+$16/1e6) title "HTM"  w yerrorbars lc 1 lw 4 ps 2, \
  L_HTM     u ($1-0.05):($8/1e6)                                   notitle      w lines      lc 1 lw 4, \
  L_PSTM    u ($1-0.02):($5/1e6):($5/1e6-$12/1e6):($5/1e6+$12/1e6) title 'PSTM' w yerrorbars lc 7 lw 4 ps 2, \
  L_PSTM    u ($1-0.02):($5/1e6)                                   notitle      w lines      lc 7 lw 4, \
  L_PHTM    u ($1-0.02):($8/1e6):($8/1e6-$16/1e6):($8/1e6+$16/1e6) title 'PHTM' w yerrorbars lc 2 lw 4 ps 2, \
  L_PHTM    u ($1-0.02):($8/1e6)                                   notitle      w lines      lc 2 lw 4, \
  L_NVHTM_B u ($1+0.02):($8/1e6):($8/1e6-$16/1e6):($8/1e6+$16/1e6) title 'HTPM' w yerrorbars lc 4 lw 4 ps 2, \
  L_NVHTM_B u ($1+0.02):($8/1e6)                                  notitle      w lines      lc 4 lw 4, \

unset multiplot
