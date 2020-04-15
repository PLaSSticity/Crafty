set term postscript color eps enhanced 22
# ARG1 - PATH_TO_DATA
# ARG2 - ACCOUNTS
# ARG3 - THREADS
set output sprintf("|ps2pdf -dEPSCrop - test_%s_PA.pdf", ARG1)

set grid ytics
set xtics norangelimit font ",8pt"
set xtics border in scale 0,0 offset -0.2,0.3 nomirror rotate by -45  autojustify

set style fill pattern 1 border lt -1
set style histogram rowstacked title font ",10" \
    textcolor lt -1 offset character 0.5, 0.4
set style data histograms

set key outside horizontal Left top left reverse samplen 1 width 1 maxrows 1 maxcols 12
set title sprintf('bank (Nb. accounts=%s, TX\_SIZE=%s, Nb. threads=%s)', \
    ARG3, ARG4, ARG5)

set ylabel "Probability of Abort"
set xlabel "Update Rate (%)"

### TODO:

NVHTM_B_10000  =sprintf("%s/test_NVHTM_B_10000.txt",   ARG2)
NVHTM_B_100000 =sprintf("%s/test_NVHTM_B_100000.txt",  ARG2)
NVHTM_B_1000000=sprintf("%s/test_NVHTM_B_1000000.txt", ARG2)
NVHTM_B_50_10000  =sprintf("%s/test_NVHTM_B_50_10000.txt",   ARG2)
NVHTM_B_50_100000 =sprintf("%s/test_NVHTM_B_50_100000.txt",  ARG2)
NVHTM_B_50_1000000=sprintf("%s/test_NVHTM_B_50_1000000.txt", ARG2)
NVHTM_F_10000  =sprintf("%s/test_NVHTM_F_10000.txt",   ARG2)
NVHTM_F_100000 =sprintf("%s/test_NVHTM_F_100000.txt",  ARG2)
NVHTM_F_1000000=sprintf("%s/test_NVHTM_F_1000000.txt", ARG2)

plot \
    newhistogram 'B 99% (10k)' lt 1 fs pattern 1 , \
		NVHTM_B_10000  using 3:xtic(1) title 'CONFLICT', \
        ''      u     4         title 'CAPACITY', \
        ''      u     6         title 'OTHER', \
	newhistogram 'B 99% (100k)' lt 1 fs pattern 1 , \
        NVHTM_B_100000  using 3:xtic(1) notitle, \
        ''      u     4         notitle, \
        ''      u     6         notitle, \
	newhistogram 'B 99% (1M)' lt 1 fs pattern 1 , \
        NVHTM_B_1000000  using 3:xtic(1) notitle, \
        ''      u     4         notitle, \
        ''      u     6         notitle, \
	newhistogram 'B 50% (10k)' lt 1 fs pattern 1 , \
        NVHTM_B_50_10000  using 3:xtic(1) notitle, \
        ''      u     4         notitle, \
        ''      u     6         notitle, \
    newhistogram 'B 50% (100k)' lt 1 fs pattern 1 , \
        NVHTM_B_50_1000000  using 3:xtic(1) notitle, \
        ''      u     4         notitle, \
        ''      u     6         notitle, \
    newhistogram 'B 50% (1M)' lt 1 fs pattern 1 , \
        NVHTM_B_50_1000000  using 3:xtic(1) notitle, \
        ''      u     4         notitle, \
        ''      u     6         notitle, \
    newhistogram 'F (10k)' lt 1 fs pattern 1 , \
        NVHTM_F_10000  using 3:xtic(1) notitle, \
        ''      u     4         notitle, \
        ''      u     6         notitle, \
    newhistogram 'F (100k)' lt 1 fs pattern 1 , \
        NVHTM_F_1000000  using 3:xtic(1) notitle, \
        ''      u     4         notitle, \
        ''      u     6         notitle, \
    newhistogram 'F (1M)' lt 1 fs pattern 1 , \
        NVHTM_F_1000000  using 3:xtic(1) notitle, \
        ''      u     4         notitle, \
        ''      u     6         notitle, \
#
