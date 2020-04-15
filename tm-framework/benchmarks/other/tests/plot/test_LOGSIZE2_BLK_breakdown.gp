set term postscript color eps enhanced 22 size 20cm,7cm
# ARG1 - PATH_TO_DATA
# ARG2 - ACCOUNTS
# ARG3 - THREADS
set output sprintf("|ps2pdf -dEPSCrop - test_%s_BLOCK_BREAKDOWN.pdf", ARG1)

set grid ytics
set noxtics

set lmargin 7.3
set rmargin 1
set tmargin 1
set bmargin 3.5

#set logscale y 2
# set format y "%.1t{/*0.8 x10}^{%T}"

set style fill pattern 2 border lt -1
set style histogram rowstacked gap 0 title font ",24" \
    textcolor lt -1 offset character 1.2, 0.8
set style data histograms

set key at graph 0.76,1 vertical Left left reverse samplen 2 width 1 \
  maxrows 2 maxcols 1 font ",30" spacing 4 width 2
#set title sprintf('bank (Nb. accounts=%s, TX\_SIZE=%s, UPDATE\_RATE=%s)', \
#    ARG3, ARG4, ARG5)

set ylabel "Ratio time" font ",34" offset 2,-0.0
#set xlabel "Update Rate (%)"

### TODO:

REMOVE_LINES="<(sed '3d;6d;7d;8d;10d;11d;12d;13d'"

PSTM     =sprintf("%s %s/test_PSTM.txt)",   REMOVE_LINES, ARG2)
PHTM     =sprintf("%s %s/test_PHTM.txt)",   REMOVE_LINES, ARG2)
NVHTM_B_1=sprintf("%s %s/test_NVHTM_B_50_1.txt)",   REMOVE_LINES, ARG2)
NVHTM_B_2=sprintf("%s %s/test_NVHTM_B_50_2.4.txt)", REMOVE_LINES, ARG2)
NVHTM_B_8=sprintf("%s %s/test_NVHTM_B_50_12.txt)",  REMOVE_LINES, ARG2)
NVHTM_F_1=sprintf("%s %s/test_NVHTM_F_1.txt)",      REMOVE_LINES, ARG2)
NVHTM_F_2=sprintf("%s %s/test_NVHTM_F_2.4.txt)",    REMOVE_LINES, ARG2)
NVHTM_F_8=sprintf("%s %s/test_NVHTM_F_12.txt)",     REMOVE_LINES, ARG2)
NVHTM_W  =sprintf("%s %s/test_NVHTM_W.txt)",        REMOVE_LINES, ARG2)

set xlabel "Number of Threads (1, 4, 8, 16, 28)" font ",24" offset 0,-1.6

plot \
	newhistogram "HTPM_{NLP}" lt -1 fs pattern 2, \
        NVHTM_W  u $(10*100):xtic(1) notitle lw 2 lc 1, \
  newhistogram "HTPM_{(85\%)}" lt -1 fs pattern 2, \
      NVHTM_B_1 u $(14*100):xtic(1) title "Wait others" lw 2 lc 1, \
      ''      u     $(9*100)        title "Log full" lw 2 lc 2, \
  newhistogram "HTPM_{(10x)}" lt -1 fs pattern 2, \
      NVHTM_B_8 u $(14*100):xtic(1) notitle lw 2 lc 1, \
      ''      u     $(9*100)        notitle lw 2 lc 2, \
  newhistogram "HTPM_{F (85\%)}" lt -1 fs pattern 2, \
      NVHTM_F_1 u $(14*100):xtic(1) notitle lw 2 lc 1, \
      ''      u     $(9*100)        notitle lw 2 lc 2, \
  newhistogram "HTPM_{F (10x)}" lt -1 fs pattern 2, \
      NVHTM_F_8 u $(14*100):xtic(1) notitle lw 2 lc 1, \
      ''        u $(9*100)          notitle lw 2 lc 2, \
#
