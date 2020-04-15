set term postscript color eps enhanced 22
# ARG1 - PATH_TO_DATA
# ARG2 - BENCHMARK NAME
# ARG3 - BENCHMARK
set output sprintf("|ps2pdf -dEPSCrop - test_%s_X.pdf", ARG2)

set grid ytics
set grid xtics

#set logscale x 2
set mxtics

### TODO:

set key font ",16" outside right Left reverse width 4
set ylabel "Throughput"
set xlabel "Number of Threads"
set title font ",18" sprintf("%s", ARG3)

STM  =sprintf("%s/test_%s_STM.txt", ARG1, ARG2)
PSTM =sprintf("%s/test_%s_PSTM.txt", ARG1, ARG2)
HTM  =sprintf("%s/test_%s_HTM.txt", ARG1, ARG2)
PHTM =sprintf("%s/test_%s_PHTM.txt", ARG1, ARG2)
NVHTM_F_LC=sprintf("%s/test_%s_NVHTM_F_LC.txt", ARG1, ARG2)
NVHTM_B=sprintf("%s/test_%s_NVHTM_B.txt", ARG1, ARG2)
NVHTM_F=sprintf("%s/test_%s_NVHTM_F.txt", ARG1, ARG2)
NVHTM_W=sprintf("%s/test_%s_NVHTM_W.txt", ARG1, ARG2)

plot \
  STM         using ($1 - 0.075):6:($6-$12):($6+$12)   notitle         w yerrorbars  lc 6, \
  STM         using ($1 - 0.075):6                     title "STM"     w linespoints lc 6, \
  PSTM        using ($1 - 0.05):6:($6-$12):($6+$12)    notitle         w yerrorbars  lc 7, \
  PSTM        using ($1 - 0.05):6                      title 'PSTM'    w linespoints lc 7, \
  HTM         using ($1 - 0.025):7:($7-$14):($7+$12)   notitle         w yerrorbars  lc 1, \
  HTM         using ($1 - 0.025):7                     title "HTM"     w linespoints lc 1, \
  PHTM        using ($1 - 0.0):7:($7-$14):($7+$14)     notitle         w yerrorbars  lc 2, \
  PHTM        using ($1 - 0.0):7                       title 'PHTM'    w linespoints lc 2, \
  NVHTM_F     using ($1 + 0.025):7:($7-$14):($7+$14)   notitle         w yerrorbars  lc 3, \
  NVHTM_F     using ($1 + 0.025):7                     title 'HTPM_F'  w linespoints lc 3, \
  NVHTM_B     using ($1 + 0.05):7:($7-$14):($7+$14)    notitle         w yerrorbars  lc 4, \
  NVHTM_B     using ($1 + 0.05):7                      title 'HTPM_B'  w linespoints lc 4, \
  NVHTM_W     using ($1 + 0.075):7:($7-$14):($7+$14)   notitle         w yerrorbars  lc 5, \
  NVHTM_W     using ($1 + 0.075):7                     title 'HTPM_W'  w linespoints lc 5, \
  