set term postscript color eps enhanced 22
# ARG1 - PATH_TO_DATA
# ARG2 - BENCHMARK NAME
# ARG3 - BENCHMARK
set output sprintf("|ps2pdf -dEPSCrop - test_%s_PA.pdf", ARG2)

set grid ytics
set xtics norangelimit font ",8pt"
set xtics border in scale 0,0 offset -0.2,0.3 nomirror rotate by -45  autojustify

set style fill pattern 1 border lt -1
set style histogram rowstacked title font ",10" \
    textcolor lt -1 offset character 0.5, 0.1
set style data histograms

set key outside horizontal Left right reverse samplen 1 width 1 maxrows 24 maxcols 1
set title font ",18" sprintf("%s", ARG3)

set ylabel "Probability of Abort"
set xlabel "Number of Threads"

### TODO:

HTM  =sprintf("%s/test_%s_HTM.txt", ARG1, ARG2)
PHTM =sprintf("%s/test_%s_PHTM.txt", ARG1, ARG2)
NVHTM_F_LC=sprintf("%s/test_%s_NVHTM_F_LC.txt", ARG1, ARG2)
NVHTM_B=sprintf("%s/test_%s_NVHTM_B.txt", ARG1, ARG2)
NVHTM_F=sprintf("%s/test_%s_NVHTM_F.txt", ARG1, ARG2)
NVHTM_W=sprintf("%s/test_%s_NVHTM_W.txt", ARG1, ARG2)

# set yrange [ 0.00000 : 900000. ] noreverse nowriteback
#
plot \
    newhistogram 'HTM' lt 1 fs pattern 1, \
		HTM   using 3:xtic(1) title 'CONFLICT', \
        ''    u     4         title 'CAPACITY', \
        ''    u     5         title 'EXPLICIT', \
        ''    u     6         title 'OTHER', \
	newhistogram 'PHTM' lt 1 fs pattern 1, \
        PHTM  using 3:xtic(1) notitle, \
        ''    u     4         notitle, \
        ''    u     5         notitle, \
        ''    u     6         notitle, \
	newhistogram 'NVHTM_F' lt 1 fs pattern 1, \
        NVHTM_F using 3:xtic(1) notitle, \
        ''      u     4         notitle, \
        ''      u     5         notitle, \
        ''      u     6         notitle, \
	newhistogram 'NVHTM_B' lt 1 fs pattern 1, \
        NVHTM_B using 3:xtic(1) notitle, \
        ''      u     4         notitle, \
        ''      u     5         notitle, \
        ''      u     6         notitle, \
	newhistogram 'NVHTM_W' lt 1 fs pattern 1, \
        NVHTM_W using 3:xtic(1) notitle, \
        ''      u     4         notitle, \
        ''      u     5         notitle, \
        ''      u     6         notitle, \
#
