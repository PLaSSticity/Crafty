set term postscript color eps enhanced 22 size 15cm,5cm
# ARG1 - PATH_TO_DATA
# ARG2 - ACCOUNTS
# ARG3 - THREADS
set output sprintf("|ps2pdf -dEPSCrop - test_PA_MULT.pdf", ARG1)

#set grid ytics
set noxtics
set ytics offset 0.8,0 font ",20"

set yrange [0:1]

set style fill pattern 1 border lt -1
set style histogram rowstacked title font ",9" \
    textcolor lt -1 offset character 0.5, -1.08
set style data histograms

#set key outside vertical Left left reverse samplen 2 width 1 \
#  maxrows 3 maxcols 2 font ",28" spacing 4 width -4
set nokey
#set title sprintf('bank (Nb. accounts=%s, TX\_SIZE=%s, Nb. threads=%s)', \
#    ARG3, ARG4, ARG5)

set ylabel "Probability of Abort" font ",23" offset 3.0,0.0
set xlabel "\n\n{/*1.8 High Contention}" offset 0,0.7 font ",16"

### TODO:

REMOVE_LINES="<(sed '3d;6d;7d;8d;10d;11d;12d;13d'"

H_PSTM    =sprintf("%s %s/test_PSTM.txt)",            REMOVE_LINES, ARG1)
H_PHTM    =sprintf("%s %s/test_PHTM.txt)",            REMOVE_LINES, ARG1)
H_HTPM_1  =sprintf("%s %s/test_NVHTM_B_50_1.txt)",    REMOVE_LINES, ARG1)
H_HTPM_2  =sprintf("%s %s/test_NVHTM_B_50_2.4.txt)",  REMOVE_LINES, ARG1)
H_HTPM_3  =sprintf("%s %s/test_NVHTM_B_50_12.txt)",   REMOVE_LINES, ARG1)
H_HTPM_F_1=sprintf("%s %s/test_NVHTM_F_1.txt)",       REMOVE_LINES, ARG1)
H_HTPM_F_2=sprintf("%s %s/test_NVHTM_F_2.4.txt)",     REMOVE_LINES, ARG1)
H_HTPM_F_3=sprintf("%s %s/test_NVHTM_F_12.txt)",      REMOVE_LINES, ARG1)
H_HTPM_W  =sprintf("%s %s/test_NVHTM_W.txt)",         REMOVE_LINES, ARG1)

L_PSTM    =sprintf("%s %s/test_PSTM.txt)",           REMOVE_LINES, ARG2)
L_PHTM    =sprintf("%s %s/test_PHTM.txt)",           REMOVE_LINES, ARG2)
L_HTPM_1  =sprintf("%s %s/test_NVHTM_B_50_1.txt)",   REMOVE_LINES, ARG2)
L_HTPM_2  =sprintf("%s %s/test_NVHTM_B_50_2.4.txt)", REMOVE_LINES, ARG2)
L_HTPM_3  =sprintf("%s %s/test_NVHTM_B_50_12.txt)",  REMOVE_LINES, ARG2)
L_HTPM_F_1=sprintf("%s %s/test_NVHTM_F_1.txt)",      REMOVE_LINES, ARG2)
L_HTPM_F_2=sprintf("%s %s/test_NVHTM_F_2.4.txt)",    REMOVE_LINES, ARG2)
L_HTPM_F_3=sprintf("%s %s/test_NVHTM_F_12.txt)",     REMOVE_LINES, ARG2)
L_HTPM_W  =sprintf("%s %s/test_NVHTM_W.txt)",        REMOVE_LINES, ARG2)

set title font ",25"

set multiplot layout 1, 2 margins 0.07,0.99,0.20,0.96 spacing 0.04

plot \
  newhistogram '{/*2 NV-HTM_{NLP}}' lt 1 fs pattern 1 , \
    H_HTPM_W u 3:xtic(1) notitle lw 2 lc 1, \
    ''       u 4         notitle  lw 2 lc 2, \
    ''       u 6         notitle  lw 2 lc 3, \
  newhistogram "        {/*2 NV-HTM_{(10x)}}" lt 1 fs pattern 1, \
	  H_HTPM_3 u 3:xtic(1) title 'Conflict'  lw 2 lc 1, \
    ''       u 4         title 'Capacity' lw 2 lc 2, \
    ''       u 6         title 'Other' lw 2 lc 3, \
  newhistogram ' {/*2 PHTM}' lt 1 fs pattern 1 , \
    H_PHTM   u 3:xtic(1) notitle lw 2 lc 1, \
    ''       u 4         notitle lw 2 lc 2, \
    ''       u 6         notitle lw 2 lc 3, \
  newhistogram '{/*2 PSTM}' lt 1 fs pattern 1 , \
    H_PSTM   u 2:xtic(1) notitle lw 2 lc 1, \
#

unset ylabel
set ytics offset 0.8,0 font ",18"

set key at graph -0.03,0.99 vertical Left left reverse samplen 2 \
  maxrows 3 maxcols 2 font ",24" spacing 0.9 width -4

set label  "Number of Threads (1, 4, 8, 16, 28)" at -15.0,-0.38 font ",24"
set xlabel "\n\n{/*1.8 Low Contention}"

plot \
newhistogram '{/*2 NV-HTM_{NLP}}' lt 1 fs pattern 1 , \
  L_HTPM_W u 3:xtic(1) notitle lw 2 lc 1, \
  ''       u 4         notitle  lw 2 lc 2, \
  ''       u 6         notitle  lw 2 lc 3, \
newhistogram "        {/*2 NV-HTM_{(10x)}}" lt 1 fs pattern 1, \
  L_HTPM_3 u 3:xtic(1) title 'Conflict'  lw 2 lc 1, \
  ''       u 4         title 'Capacity' lw 2 lc 2, \
  ''       u 6         title 'Other' lw 2 lc 3, \
newhistogram ' {/*2 PHTM}' lt 1 fs pattern 1 , \
  L_PHTM   u 3:xtic(1) notitle lw 2 lc 1, \
  ''       u 4         notitle lw 2 lc 2, \
  ''       u 6         notitle lw 2 lc 3, \
newhistogram '{/*2 PSTM}' lt 1 fs pattern 1 , \
  L_PSTM   u 2:xtic(1) notitle lw 2 lc 1, \
#

unset multiplot
