set term postscript color eps enhanced 22 size 15cm,5cm
# ARG1 - FOLDER HIGH
# ARG2 - FOLDER LOW
set output sprintf("|ps2pdf -dEPSCrop - test_X_MULT.pdf")

#set grid ytics
#set grid xtics

set lmargin 6.8
set rmargin 3.3
set tmargin 10
set bmargin 1.5

#set logscale y 10
set mxtics
#set format y "%.2tx10^{%T}"
set xtics 5,5,35 rotate by -45 left font ",30" offset -0.8,0.4
set ytics 0,1,10 offset 0.8,0 font ",30"

set xrange [0:29]
set yrange [0:2.5]

### TODO:
unset xlabel
set key at graph 1.76,0.99 right Left reverse maxcols 2 maxrows 4 width 4.5 font ",16" spacing 1
#set nokey
set ylabel "Throughput ({/*0.8 x10}^6 TXs/s)" font ",26" offset 1.55,-0.7
#set xlabel "High Contention" font ",34" offset -0.0,0.0
#set title sprintf('bank (-a=%s, -s=%s, -u=%s, -r=%s, -d=%s)', \
#    ARG3, ARG4, ARG5, ARG6, ARG7)

H_PSTM  =sprintf("%s/test_PSTM.txt",            ARG1)
H_PHTM  =sprintf("%s/test_PHTM.txt",            ARG1)
H_HTPM_1=sprintf("%s/test_NVHTM_B_50_1.txt",    ARG1)
H_HTPM_2=sprintf("%s/test_NVHTM_B_50_2.4.txt",  ARG1)
H_HTPM_3=sprintf("%s/test_NVHTM_B_50_12.txt",   ARG1)
H_HTPM_F_1=sprintf("%s/test_NVHTM_F_1.txt",     ARG1)
H_HTPM_F_2=sprintf("%s/test_NVHTM_F_2.4.txt",   ARG1)
H_HTPM_F_3=sprintf("%s/test_NVHTM_F_12.txt",    ARG1)
H_HTPM_F_4=sprintf("%s/test_NVHTM_F_NEW_12.txt",    ARG1)
H_HTPM_W=sprintf("%s/test_NVHTM_W.txt",         ARG1)

L_PSTM  =sprintf("%s/test_PSTM.txt",           ARG2)
L_PHTM  =sprintf("%s/test_PHTM.txt",           ARG2)
L_HTPM_1=sprintf("%s/test_NVHTM_B_50_1.txt",   ARG2)
L_HTPM_2=sprintf("%s/test_NVHTM_B_50_2.4.txt", ARG2)
L_HTPM_3=sprintf("%s/test_NVHTM_B_50_12.txt",  ARG2)
L_HTPM_F_1=sprintf("%s/test_NVHTM_F_1.txt",    ARG2)
L_HTPM_F_2=sprintf("%s/test_NVHTM_F_2.4.txt",  ARG2)
L_HTPM_F_3=sprintf("%s/test_NVHTM_F_12.txt",   ARG2)
L_HTPM_F_4=sprintf("%s/test_NVHTM_F_NEW_12.txt",   ARG2)
L_HTPM_W=sprintf("%s/test_NVHTM_W.txt",        ARG2)

set multiplot layout 1, 2 margins 0.07,0.99,0.15,0.96 spacing 0.04

plot \
  H_HTPM_1 u ($1-0.25):($7/1e6):($7/1e6-$21/1e6):($7/1e6+$21/1e6) notitle               lc 3 pt 4  ps 2   lw 4 w yerrorbars, \
  H_HTPM_1 u ($1-0.25):($7/1e6)                                   title "NV-HTM_{(x1)}" lc 3 pt 4  ps 2   lw 4 w linespoints, \
  H_HTPM_3 u ($1-0.05):($7/1e6):($7/1e6-$21/1e6):($7/1e6+$21/1e6) notitle               lc 10 pt 8 ps 2   lw 4 w yerrorbars, \
  H_HTPM_3 u ($1-0.05):($7/1e6)                                  title "NV-HTM_{(x10)}" lc 10 pt 8 ps 2   lw 4 w linespoints, \
  H_HTPM_F_4 u ($1-0.05):($7/1e6):($7/1e6-$21/1e6):($7/1e6+$21/1e6) notitle             lc 2 pt 7 ps 1.6 lw 4 w yerrorbars, \
  H_HTPM_F_4 u ($1-0.05):($7/1e6)                     title "NV-HTM_{FFF (x10)}" dt "." lc 2 pt 7 ps 1.6 lw 4 w linespoints, \
  H_HTPM_F_3 u ($1-0.05):($7/1e6):($7/1e6-$21/1e6):($7/1e6+$21/1e6) notitle             lc 2 pt 6 ps 1.6 lw 4 w yerrorbars, \
  H_HTPM_F_3 u ($1-0.05):($7/1e6)                     title "NV-HTM_{FNF (x10)}" dt "." lc 2 pt 6 ps 1.6 lw 4 w linespoints, \
  H_HTPM_W u ($1+0.05):($7/1e6):($7/1e6-$16/1e6):($7/1e6+$16/1e6) notitle               lc 1 pt 1  ps 2   lw 4 w yerrorbars, \
  H_HTPM_W u ($1+0.05):($7/1e6)                             title "NV-HTM_{NLP}" dt "." lc 1 pt 1  ps 2   lw 4 w linespoints, \
  H_PSTM   u ($1+0.25):($3/1e6):($3/1e6-$8/1e6):($3/1e6+$8/1e6)   notitle               lc 4 pt 11 ps 2   lw 4 w yerrorbars, \
  H_PSTM   u ($1+0.25):($3/1e6)                                   title "PSTM" dt "-.-" lc 4 pt 11 ps 2   lw 4 w linespoints, \
  H_PHTM   u ($1+0.15):($7/1e6):($7/1e6-$16/1e6):($7/1e6+$16/1e6) notitle               lc 7 pt 2  ps 2   lw 4 w yerrorbars, \
  H_PHTM   u ($1+0.15):($7/1e6)                                   title "PHTM" dt "-.-" lc 7 pt 2  ps 2   lw 4 w linespoints, \

unset key
#unset ytics
unset ylabel
set yrange [0:5.5]

#set xlabel "Low Contention" font ",34" offset -0.0,0.0

plot \
  L_HTPM_1 u ($1-0.25):($7/1e6):($7/1e6-$21/1e6):($7/1e6+$21/1e6) notitle               lc 3 pt 4  ps 2   lw 4 w yerrorbars, \
  L_HTPM_1 u ($1-0.25):($7/1e6)                                 title "NV-HTM_{(x1)}"   lc 3 pt 4  ps 2   lw 4 w linespoints, \
  L_HTPM_3 u ($1-0.05):($7/1e6):($7/1e6-$21/1e6):($7/1e6+$21/1e6) notitle               lc 10 pt 8 ps 2   lw 4 w yerrorbars, \
  L_HTPM_3 u ($1-0.05):($7/1e6)                                 title "NV-HTM_{(x10)}"  lc 10 pt 8 ps 2   lw 4 w linespoints, \
  L_HTPM_F_4 u ($1-0.05):($7/1e6):($7/1e6-$21/1e6):($7/1e6+$21/1e6) notitle             lc 2 pt 7 ps 1.6 lw 4 w yerrorbars, \
  L_HTPM_F_4 u ($1-0.05):($7/1e6)                     title "NV-HTM_{FFF (x10)}" dt "." lc 2 pt 7 ps 1.6 lw 4 w linespoints, \
  L_HTPM_F_3 u ($1-0.05):($7/1e6):($7/1e6-$21/1e6):($7/1e6+$21/1e6) notitle             lc 2 pt 6 ps 1.6 lw 4 w yerrorbars, \
  L_HTPM_F_3 u ($1-0.05):($7/1e6)                     title "NV-HTM_{FNF (x10)}" dt "." lc 2 pt 6 ps 1.6 lw 4 w linespoints, \
  L_HTPM_W u ($1+0.05):($7/1e6):($7/1e6-$17/1e6):($7/1e6+$17/1e6) notitle               lc 1 pt 1  ps 2   lw 4 w yerrorbars, \
  L_HTPM_W u ($1+0.05):($7/1e6)                             title "NV-HTM_{NLP}" dt "." lc 1 pt 1  ps 2   lw 4 w linespoints, \
  L_PSTM   u ($1+0.25):($3/1e6):($3/1e6-$8/1e6):($3/1e6+$8/1e6)   notitle               lc 4 pt 11 ps 2   lw 4 w yerrorbars, \
  L_PSTM   u ($1+0.25):($3/1e6)                                   title "PSTM" dt "-.-" lc 4 pt 11 ps 2   lw 4 w linespoints, \
  L_PHTM   u ($1+0.15):($7/1e6):($7/1e6-$16/1e6):($7/1e6+$16/1e6) notitle               lc 7 pt 2  ps 2   lw 4 w yerrorbars, \
  L_PHTM   u ($1+0.15):($7/1e6)                                   title "PHTM" dt "-.-" lc 7 pt 2  ps 2   lw 4 w linespoints, \

unset multiplot
