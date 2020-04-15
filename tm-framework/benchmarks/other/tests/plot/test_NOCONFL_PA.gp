set term postscript color eps enhanced 22 size 15cm,6.5cm
# ARG1 - NAME PDF
# ARG2 - PATH_TO_DATA
# ARG3 - ACCOUNTS
# ARG4 - TX_SIZE
set output sprintf("|ps2pdf -dEPSCrop - test_%s_PA.pdf", ARG1)

#set grid ytics
set xtics norangelimit font ",22pt"
set xtics border in scale 0,0 offset 0.8,0.2 nomirror rotate by 45 right
set ytics font ",28pt" offset 0.4,0.0

set lmargin 7.3
set rmargin 0.4
set tmargin 0.7
set bmargin 4.0

set yrange [0:1]

set style fill pattern 1 border lt -1
set style histogram rowstacked title font ",10" \
    textcolor lt -1 offset character 0.5, -0.8
set style data histograms

set key at graph 0,1.0 vertical Left top left reverse samplen 2 width -1 \
  maxrows 3 maxcols 1 font ",48" spacing 1
#set title sprintf('bank (Nb. accounts=%s, TX\_SIZE=%s)', ARG3, ARG4)

set ylabel "Probability of Abort" font ",34pt" offset 1,0
set xlabel "\n{/*1 Number of Threads}\n(1, 2, 4, 6, 8, 12, 16, 20, 24, 28)"    font ",30pt" offset 0,0.6

### TODO:
HTM  =sprintf("%s/test_HTM_%s.txt", ARG2, ARG1)
NVHTM=sprintf("%s/test_NVHTM_%s.txt", ARG2, ARG1)
NVHTM_LC=sprintf("%s/test_NVHTM_LC_%s.txt", ARG2, ARG1)
PHTM =sprintf("%s/test_PHTM_%s.txt", ARG2, ARG1)
NVHTM_W=sprintf("%s/test_NVHTM_W_%s.txt", ARG2, ARG1)
NVHTM_LC_W=sprintf("%s/test_NVHTM_LC_W_%s.txt", ARG2, ARG1)
NVHTM_B=sprintf("%s/test_NVHTM_B_%s.txt", ARG2, ARG1)
NVHTM_LC_B=sprintf("%s/test_NVHTM_LC_B_%s.txt", ARG2, ARG1)

unset xtics
#set label  "Number of Threads" center at graph 0.5,-0.25 font ",24"

plot \
	newhistogram "\n{/*2.6 NV-HTM^{PC}}" lt -1 fs pattern 1 , \
        NVHTM_W using 3:xtic(1) title "{/*0.9 Conflict}" lw 2 lc 1, \
        ''      u     4         title '{/*0.9 Capacity}' lw 2 lc 2, \
        ''      u     5         title '{/*0.9 Other}'  lw 2 lc 3, \
	newhistogram "\n{/*2.6 NV-HTM^{LC}}" lt -1 fs pattern 1 , \
        NVHTM_LC_W u 3:xtic(1) notitle lw 2 lc 1, \
        ''         u     4     notitle lw 2 lc 2, \
        ''         u     5     notitle lw 2 lc 3, \
#
