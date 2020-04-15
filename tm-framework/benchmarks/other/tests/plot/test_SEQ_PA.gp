set term postscript color eps enhanced 22
# ARG1 - NAME PDF
# ARG2 - PATH_TO_DATA
# ARG3 - ACCOUNTS
# ARG4 - TX_SIZE
# ARG5 - READ_SIZE
# ARG6 - THREADS
set output sprintf("|ps2pdf -dEPSCrop - test_%s_PA.pdf", ARG1)

set grid ytics
set xtics norangelimit font ",8pt"
set xtics border in scale 0,0 offset -0.2,0.3 nomirror rotate by -45  autojustify

set style fill pattern 1 border lt -1
set style histogram rowstacked title font ",10" \
    textcolor lt -1 offset character 0.5, 0.8
set style data histograms

set key outside horizontal Left top left reverse samplen 1 width 1 maxrows 1 maxcols 12
set title sprintf('bank (ACCOUNTS=%s, TX\_SIZE=%s, READ\_SIZE=%s, THREADS=%s)', \
    ARG3, ARG4, ARG5, ARG6) font ",16"

set ylabel "Probability of Abort"
set xlabel "% Written Accounts"

### TODO:
HTM  =sprintf("%s/test_HTM_%s.txt", ARG2, ARG1)
NVHTM=sprintf("%s/test_NVHTM_%s.txt", ARG2, ARG1)
NVHTM_LC=sprintf("%s/test_NVHTM_LC_%s.txt", ARG2, ARG1)
PHTM =sprintf("%s/test_PHTM_%s.txt", ARG2, ARG1)
NVHTM_W=sprintf("%s/test_NVHTM_W_%s.txt", ARG2, ARG1)
NVHTM_LC_W=sprintf("%s/test_NVHTM_LC_W_%s.txt", ARG2, ARG1)

plot \
    newhistogram 'HTM' lt 1 fs pattern 1 , \
		HTM  using 3:xtic(1) title 'CONFL', \
        ''      u     4         title 'CAPAC', \
        ''      u     5         title 'OTHER', \
	newhistogram 'NVHTM' lt 1 fs pattern 1 , \
        NVHTM  using 3:xtic(1) notitle, \
        ''      u     4         notitle, \
        ''      u     5         notitle, \
	newhistogram 'NVHTM (LC)' lt 1 fs pattern 1 , \
        NVHTM_LC  using 3:xtic(1) notitle, \
        ''      u     4         notitle, \
        ''      u     5         notitle, \
	newhistogram 'PHTM' lt 1 fs pattern 1 , \
        PHTM  using 3:xtic(1) notitle, \
        ''      u     4         notitle, \
        ''      u     5         notitle, \
	newhistogram 'NVHTM_W' lt 1 fs pattern 1 , \
        NVHTM_W using 3:xtic(1) notitle, \
        ''      u     4         notitle, \
        ''      u     5         notitle, \
	newhistogram 'NVHTM_W (LC)' lt 1 fs pattern 1 , \
        NVHTM_LC_W using 3:xtic(1) notitle, \
        ''      u     4         notitle, \
        ''      u     5         notitle, \
#
