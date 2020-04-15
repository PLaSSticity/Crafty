set term postscript color eps enhanced 22 size 15cm,6.5cm
# ARG1 - PATH_TO_DATA
set output sprintf("|ps2pdf -dEPSCrop - test_PA_MULT.pdf", ARG2)

#set grid ytics
set xtics norangelimit font ",8pt"
set xtics border in scale 0,0 offset -0.2,0.4 nomirror rotate by -45 \
  autojustify font ",11"
set ytics offset 0.4,0 font ",22"

set yrange [0:1]

set style fill pattern 1 border lt -1
set style histogram rowstacked gap 0 title font ",10" \
    textcolor lt -1 offset character 0.5,-1.6
set style data histograms

#set key outside horizontal Left top left reverse samplen 1 width 1 maxrows 1 maxcols 12
set nokey
#set title font ",18" sprintf("%s", ARG3)

set ylabel "Probability of Abort" font ",30" offset 2.2,-0.0
set xlabel "\n\n\n{/*2 TPC-C (high)}" offset 0,1.0 font ",16"

set yrange [0:1]

### TODO:

REMOVE_LINES="<(sed '3d;6d;8d;9d'"

H_STM    =sprintf("%s %s/test_TPCC_H_STM.txt)",    REMOVE_LINES, ARG1)
H_PSTM   =sprintf("%s %s/test_TPCC_H_PSTM.txt)",    REMOVE_LINES, ARG1)
H_HTM    =sprintf("%s %s/test_TPCC_H_HTM.txt)",     REMOVE_LINES, ARG1)
H_PHTM   =sprintf("%s %s/test_TPCC_H_PHTM.txt)",    REMOVE_LINES, ARG1)
H_NVHTM_B=sprintf("%s %s/test_TPCC_H_NVHTM_B.txt)", REMOVE_LINES, ARG1)

L_STM    =sprintf("%s %s/test_TPCC_L_STM.txt)",     REMOVE_LINES, ARG1)
L_PSTM   =sprintf("%s %s/test_TPCC_L_PSTM.txt)",    REMOVE_LINES, ARG1)
L_HTM    =sprintf("%s %s/test_TPCC_L_HTM.txt)",     REMOVE_LINES, ARG1)
L_PHTM   =sprintf("%s %s/test_TPCC_L_PHTM.txt)",    REMOVE_LINES, ARG1)
L_NVHTM_B=sprintf("%s %s/test_TPCC_L_NVHTM_B.txt)", REMOVE_LINES, ARG1)

set multiplot layout 1, 2 margins 0.09,0.99,0.3,0.97 spacing 0.01

unset xtics

# set yrange [ 0.00000 : 900000. ] noreverse nowriteback
#
plot \
  newhistogram "{/*2 HTM}" lt 1 fs pattern 1, \
	 H_HTM u 3:xtic(1) notitle lw 2 lc 1, \
    '' u 4         notitle lw 2 lc 2, \
    '' u 7         notitle lw 2 lc 3, \
  newhistogram "{/*2 HTPM}" lt 1 fs pattern 1, \
    H_NVHTM_B u 3:xtic(1) title 'Conflict'                                     lw 2 lc 1, \
    ''      u 4         title 'Capacity'                                       lw 2 lc 2, \
    ''      u 6         title 'Unspecified'                                    lw 2 lc 3, \
    ''      u 5         title "{/*0.8 Log full (HTPM)}\n{/*0.8 Locked (PHTM)}" lw 2 lc 7, \
  newhistogram "{/*2 PHTM}" lt 1 fs pattern 1, \
    H_PHTM u 3:xtic(1) notitle lw 2 lc 1, \
    ''   u 4         notitle lw 2 lc 2, \
    ''   u 6         notitle lw 2 lc 3, \
    ''   u 5         notitle lw 2 lc 7, \
	newhistogram "{/*2 STM}" lt 1 fs pattern 1, \
    H_STM     u 2:xtic(1) notitle lw 2 lc 1, \
	newhistogram "{/*2 PSTM}" lt 1 fs pattern 1, \
    H_PSTM    u 2:xtic(1) notitle lw 2 lc 1, \
#

unset key
unset ylabel
unset ytics

set key at graph 0,1.0 horizontal Left top left reverse samplen 1 width 1 \
  maxrows 3 maxcols 12 font "Helvetica,10" spacing 4.5

set xlabel "\n\n\n{/*2 TPC-C (low)}"
set label  "Number of Threads (1, 4, 8, 16, 28)" at -17.0,-0.38 font ",24"

plot \
  newhistogram "{/*2 HTM}" lt 1 fs pattern 1, \
	 L_HTM u 3:xtic(1) notitle lw 2 lc 1, \
    '' u 4         notitle lw 2 lc 2, \
    '' u 7         notitle lw 2 lc 3, \
  newhistogram "{/*2 HTPM}" lt 1 fs pattern 1, \
    L_NVHTM_B u 3:xtic(1) title '{/*2.5 Conflict}'                             lw 2 lc 1, \
    ''      u 4         title '{/*2.5 Capacity}'                               lw 2 lc 2, \
    ''      u 6         title '{/*2.5 Unspecified}'                            lw 2 lc 3, \
    ''      u 5         title "{/*1.8 Log full (HTPM)}\n\n{/*1.8 Locked (PHTM)}" lw 2 lc 7, \
  newhistogram "{/*2 PHTM}" lt 1 fs pattern 1, \
    L_PHTM u 3:xtic(1) notitle lw 2 lc 1, \
    ''   u 4         notitle lw 2 lc 2, \
    ''   u 6         notitle lw 2 lc 3, \
    ''   u 5         notitle lw 2 lc 7, \
	newhistogram "{/*2 STM}" lt 1 fs pattern 1, \
    L_STM     u 2:xtic(1) notitle lw 2 lc 1, \
	newhistogram "{/*2 PSTM}" lt 1 fs pattern 1, \
    L_PSTM    u 2:xtic(1) notitle lw 2 lc 1, \
#


unset multiplot
