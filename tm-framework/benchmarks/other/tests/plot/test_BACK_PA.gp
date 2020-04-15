set term postscript color eps enhanced 22
# ARG1 - PATH_TO_DATA
# ARG2 - ACCOUNTS
# ARG3 - THREADS
set output sprintf("|ps2pdf -dEPSCrop - test_BACKWARD_PA_R%s.pdf", ARG4)

set grid ytics
set xtics norangelimit font ",8pt"
set xtics border in scale 0,0 offset -0.2,0.3 nomirror rotate by -45  autojustify

set style fill pattern 1 border lt -1
set style histogram rowstacked title font ",10" \
    textcolor lt -1 offset character 0.5, 0.8
set style data histograms

set key outside horizontal Left top left reverse samplen 1 width 1 maxrows 1 maxcols 12
set title sprintf("bank (Nb. accounts=%s, THREADS=%s)", ARG2, ARG3, ARG4)

set ylabel "Probability of Abort"
set xlabel "% Written Accounts"

### TODO:

NVHTM1=sprintf("%s/test_NVHTM0_01_%s_BACK.txt", ARG1, ARG4)
NVHTM2=sprintf("%s/test_NVHTM0_05_%s_BACK.txt", ARG1, ARG4)
NVHTM3=sprintf("%s/test_NVHTM0_1_%s_BACK.txt", ARG1, ARG4)
NVHTM4=sprintf("%s/test_NVHTM0_5_%s_BACK.txt", ARG1, ARG4)
NVHTM_F=sprintf("%s/test_NVHTM_F_%s_BACK.txt", ARG1, ARG4)
NVHTM_W=sprintf("%s/test_NVHTM_W_%s_BACK.txt", ARG1, ARG4)
PHTM=sprintf("%s/test_PHTM_%s_BACK.txt", ARG1, ARG4)
HTM=sprintf("%s/test_HTM_%s_BACK.txt", ARG1, ARG4)

plot \
    newhistogram 'NVHTM_B 0.01' lt 1 fs pattern 1 , \
		NVHTM1  using 3:xtic(1) title 'CONFL', \
        ''      u     4         title 'CAPAC', \
        ''      u     5         title 'OTHER', \
	newhistogram 'NVHTM_B 0.05' lt 1 fs pattern 1 , \
        NVHTM2  using 3:xtic(1) notitle, \
        ''      u     4         notitle, \
        ''      u     5         notitle, \
	newhistogram 'NVHTM_B 0.1' lt 1 fs pattern 1 , \
        NVHTM3  using 3:xtic(1) notitle, \
        ''      u     4         notitle, \
        ''      u     5         notitle, \
	newhistogram 'NVHTM_B 0.5' lt 1 fs pattern 1 , \
        NVHTM4  using 3:xtic(1) notitle, \
        ''      u     4         notitle, \
        ''      u     5         notitle, \
	newhistogram 'NVHTM_F' lt 1 fs pattern 1 , \
        NVHTM_F using 3:xtic(1) notitle, \
        ''      u     4         notitle, \
        ''      u     5         notitle, \
	newhistogram 'NVHTM_W' lt 1 fs pattern 1 , \
        NVHTM_W using 3:xtic(1) notitle, \
        ''      u     4         notitle, \
        ''      u     5         notitle, \
	newhistogram 'PHTM' lt 1 fs pattern 1 , \
        PHTM    using 3:xtic(1) notitle, \
        ''      u     4         notitle, \
        ''      u     5         notitle, \
	newhistogram 'HTM' lt 1 fs  pattern 1 , \
        HTM    using 3:xtic(1)  notitle, \
        ''      u     4         notitle, \
        ''      u     5         notitle, \
#
