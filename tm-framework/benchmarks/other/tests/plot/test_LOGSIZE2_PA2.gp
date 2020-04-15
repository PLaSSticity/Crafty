set term postscript color eps enhanced 22 size 15cm,9cm
set output sprintf("|ps2pdf -dEPSCrop - test_%s_PA2.pdf", ARG1)

set grid ytics
set xtics norangelimit font ",8pt"
set xtics border in scale 0,0 offset -0.2,0.3 nomirror rotate by -45  autojustify
set ytics offset 0.4,0 font ",22"

set yrange [0:1]

set style fill pattern 1 border lt -1
set style histogram rowstacked title font ",10" \
    textcolor lt -1 offset character 0.5, 0.4
set style data histograms

#set key outside vertical Left left reverse samplen 2 width 1 \
#  maxrows 3 maxcols 2 font ",28" spacing 4 width -4
set nokey
#set title sprintf('bank (Nb. accounts=%s, TX\_SIZE=%s, Nb. threads=%s)', \
#    ARG3, ARG4, ARG5)

set ylabel "Probability of Abort" font ",34" offset 0.8,-0.0
set xlabel "\n\n{/*2 Number of Threads}" offset 0,0 font ",16"

### TODO:

call sprintf("%s/IMPORT_files.inc", ARG3) ARG2

set title font ",25"

plot \
  newhistogram '{/*2 HTPM_W}' lt 1 fs pattern 1 , \
        NVHTM_W  using 3:xtic(1) notitle lw 2 lc 1, \
        ''      u     4         notitle  lw 2 lc 2, \
        ''      u     6         notitle  lw 2 lc 3, \
        ''      u     5         notitle  lw 2 lc 7, \
  newhistogram "{/*2 HTPM}\n\n{/*2(1 wrap)}" lt 1 fs pattern 1, \
	 NVHTM_B_1  using 3:xtic(1) title 'Conflict'  lw 2 lc 1, \
      ''      u     4         title 'Capacity' lw 2 lc 2, \
      ''      u     6         title 'Unspecified' lw 2 lc 3, \
      ''      u     5         title "{/*0.8 Log full (HTPM)}\n{/*0.8 Locked (PHTM)}" lw 2 lc 7, \
	newhistogram "{/*2 HTPM}\n\n{/*2 (2.4 wraps)}" lt 1 fs pattern 1, \
        NVHTM_B_2  using 3:xtic(1) notitle  lw 2 lc 1, \
        ''      u     4         notitle lw 2 lc 2, \
        ''      u     6         notitle lw 2 lc 3, \
        ''      u     5         notitle lw 2 lc 7, \
	newhistogram "{/*2 HTPM}\n\n{/*2 (12 wraps)}" lt 1 fs pattern 1 , \
        NVHTM_B_8  using 3:xtic(1) notitle  lw 2 lc 1, \
        ''      u     4         notitle lw 2 lc 2, \
        ''      u     6         notitle lw 2 lc 3, \
        ''      u     5         notitle lw 2 lc 7, \
  newhistogram '{/*2 PHTM}' lt 1 fs pattern 1 , \
        PHTM  using 3:xtic(1) notitle lw 2 lc 1, \
        ''      u     4         notitle lw 2 lc 2, \
        ''      u     6         notitle lw 2 lc 3, \
        ''      u     5         notitle lw 2 lc 7, \
  #newhistogram '{/*2 PSTM}' lt 1 fs pattern 1 , \
  #      PSTM  using 2:xtic(1) notitle, \
  #newhistogram 'STM' lt 1 fs pattern 1 , \
  #STM  using 2:xtic(1) notitle, \
  #      newhistogram 'HTM' lt 1 fs pattern 1 , \
  #      HTM  using 3:xtic(1) notitle, \
  #      ''      u     4         notitle, \
  #      ''      u     6         notitle, \
#
