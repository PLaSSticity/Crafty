set term postscript color eps enhanced 22 size 15cm,9cm
# ARG1 - PATH_TO_DATA
# ARG2 - BENCHMARK NAME
# ARG3 - BENCHMARK
set output sprintf("|ps2pdf -dEPSCrop - test_%s_PA.pdf", ARG2)

set grid ytics
set xtics norangelimit font ",8pt"
set xtics border in scale 0,0 offset -0.2,0.3 nomirror rotate by -45 \
  autojustify font ",11"
set ytics offset 0.4,0 font ",22"

set yrange [0:1]

set style fill pattern 1 border lt -1
set style histogram rowstacked title font ",10" \
    textcolor lt -1 offset character 0.5, 0.1
set style data histograms

set key outside horizontal Left top left reverse samplen 1 width 1 maxrows 1 maxcols 12
#set nokey
#set title font ",18" sprintf("%s", ARG3)

set ylabel "Probability of Abort" font ",34" offset 0.8,-0.0
set xlabel "\n\n{/*2 Number of Threads}" offset 0,0 font ",16"

set yrange [0:1]

### TODO:

STM  =sprintf("%s/test_%s_STM.txt", ARG1, ARG2)
PSTM =sprintf("%s/test_%s_PSTM.txt", ARG1, ARG2)
HTM  =sprintf("%s/test_%s_HTM.txt", ARG1, ARG2)
PHTM =sprintf("%s/test_%s_PHTM.txt", ARG1, ARG2)
NVHTM_F_LC=sprintf("%s/test_%s_NVHTM_F_LC.txt", ARG1, ARG2)
NVHTM_B=sprintf("%s/test_%s_NVHTM_B.txt", ARG1, ARG2)
NVHTM_F=sprintf("%s/test_%s_NVHTM_F.txt", ARG1, ARG2)
NVHTM_W=sprintf("%s/test_%s_NVHTM_W.txt", ARG1, ARG2)

# set yrange [ 0.00000 : 900000. ] noreverse nowriteback
#
plot \
  newhistogram "{/*2 HTM}" lt 1 fs pattern 1, \
	 HTM u 3:xtic(1) notitle lw 2 lc 1, \
    '' u 4         notitle lw 2 lc 2, \
    '' u 7         notitle lw 2 lc 3, \
  newhistogram "{/*2 HTPM}" lt 1 fs pattern 1, \
    NVHTM_B u 3:xtic(1) title 'Conflict'    lw 2 lc 1, \
    ''      u 4         title 'Capacity'    lw 2 lc 2, \
    ''      u 6         title 'Unspecified' lw 2 lc 3, \
    ''      u 5         title "{/*0.8 Log full (HTPM)}\n{/*0.8 Locked (PHTM)}" lw 2 lc 7, \
  newhistogram "{/*2 PHTM}" lt 1 fs pattern 1, \
    PHTM u 3:xtic(1) notitle lw 2 lc 1, \
    ''   u 4         notitle lw 2 lc 2, \
    ''   u 6         notitle lw 2 lc 3, \
    ''   u 5         notitle lw 2 lc 7, \
	newhistogram "{/*2 STM}" lt 1 fs pattern 1, \
    STM     u 2:xtic(1) notitle lw 2 lc 1, \
	newhistogram "{/*2 PSTM}" lt 1 fs pattern 1, \
    PSTM    u 2:xtic(1) notitle lw 2 lc 1, \
    #newhistogram {/*2 NVHTM_W} lt 1 fs pattern 1, \
    #NVHTM_W u 3:xtic(1) notitle lw 2 lc 1, \
    #''      u 4         notitle lw 2 lc 2, \
    #''      u 6         notitle lw 2 lc 3, \
    #''      u 5         notitle lw 2 lc 7, \
#
