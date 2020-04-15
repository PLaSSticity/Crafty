set term postscript color eps enhanced 22 size 15cm,7cm
# ARG1 - PATH_TO_DATA
# ARG2 - ACCOUNTS
# ARG3 - THREADS
set output sprintf("|ps2pdf -dEPSCrop - test_CAP_PA2_thr%s.pdf", ARG3)

set grid ytics
set xtics norangelimit font ",13"
set xtics border in scale 0,0 offset -0.2,0.3 nomirror rotate by -45 left
set ytics offset 0.6,0 font ",28"

set lmargin 6.6
set rmargin 0.4
set tmargin 0.7
set bmargin 4.8

set style fill pattern 4 border lt -1
set style histogram rowstacked title font ",10" \
    textcolor lt -1 offset character 0.5, 0.6
set style data histograms

set key at graph 0.24,1.00 vertical Left top reverse font ",34"\
  samplen 1.4 width 0 maxrows 12 maxcols 1 spacing 1
#set title sprintf('bank (Nb. accounts=%s, TX\_SIZE=%s, Nb. threads=%s)', \
#    ARG3, ARG4, ARG5)

set ylabel "Probability of Abort" offset 1.8,0.0  font ",30"
set xlabel "\n{/*2 Number of Cache Lines}\n\n{/*2 (2, 32, 64, 80, 96, 128, 256, 512)}" offset 0,-0.8 font ",14"

### TODO:

REMOVE_LINES="<(sed '3d;4d;5d;6d;8d'"
HTM  =sprintf("%s %s/test_HTM_thr%s_CAP.txt)",   REMOVE_LINES, ARG2, ARG3)
PHTM =sprintf("%s %s/test_PHTM_thr%s_CAP.txt)",  REMOVE_LINES, ARG2, ARG3)
NVHTM=sprintf("%s %s/test_NVHTM_thr%s_CAP.txt)", REMOVE_LINES, ARG2, ARG3)

unset xtics

plot \
  newhistogram "\n{/*2.3 HTM}" lt -1 fs pattern 1, \
    HTM u 3:xtic(1) notitle lw 2 lc 1, \
    ''  u 4         notitle lw 2 lc 2, \
    ''  u 6         notitle lw 2 lc 3, \
  newhistogram "\n{/*2.3 NV-HTM}" lt -1 fs pattern 1, \
  	NVHTM  u 3:xtic(1) title "{/*0.83 Conflict}"    lw 2 lc 1, \
    ''     u 4         title "{/*0.83 Capacity}"    lw 2 lc 2, \
    ''     u 6         title "{/*0.83 Other}"     lw 2 lc 3, \
  newhistogram "\n{/*2.3 PHTM}" lt -1 fs pattern 1, \
    PHTM   u 3:xtic(1) notitle lw 2 lc 1, \
    ''     u 4         notitle lw 2 lc 2, \
    ''     u 6         notitle lw 2 lc 3, \
#
