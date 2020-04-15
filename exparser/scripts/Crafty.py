import exparser
from pathlib import Path
import scipy.stats as ss
import matplotlib.pyplot as plt
import matplotlib.patheffects as path_effects
import matplotlib.ticker as ticker
import numpy as np
import sys


#plt.rc('text', usetex=True)
#plt.rc('font', family='serif')
#plt.rc('pdf', fonttype=42)
#plt.rc('ps', fonttype=42)
#plt.rc('ps', usedistiller='xpdf')

# https://github.com/matplotlib/matplotlib/issues/8289

PRINT_TO_FILE = True
SHOW_GRAPHS = False

if len(sys.argv) != 2:
    print("Pass the experiment name to script.", file=sys.stderr)
    exit(1)
print(f'Processing experiment {sys.argv[1]}')

THREAD_COUNTS = [1, 2, 4, 8, 12, 15, 16]

OUTPUT_PATH = Path.home() / 'results'
INPUT_PATH = Path.home() / 'results' / sys.argv[1]

benchmarks = [
     'bayes',
     'genome',
     'intruder',
     'kmeans-high',
     'kmeans-low',
     'labyrinth',
     'ssca2',
     'vacation-high',
     'vacation-low',
#     'yada',
     'bank-fee',
     'bank-fee-hc',
     'bank-fee-nc',
     'bplustree',
     'bplustree-wronly',
]
#benchmarks = ['memcached']

solutions = [
    'HTM-only',
    'DudeTM',
    'NV-HTM',
    'Crafty',
    'Crafty-NoValidate',
    'Crafty-NoRedo',
]

time_regex = r'Time *([0-9]+[.][0-9]+) s'
commits_regex = r' *COMMIT *: *([0-9]+)'
fallbacks_regex = r' *FALLBK *: *([0-9]+)'
conflicts_regex = r' *CONFLS *: *([0-9]+)'
capacity_regex = r' *CAPACS *: *([0-9]+)'
explicit_regex = r' *EXPLIC *: *([0-9]+)'
zero_regex = r' *ZERO *: *([0-9]+)'
aborts_regex = r'ABORTS *: *([0-9]+)'
abort_percentage_regex = r' *P_A *: *([0-9]+[.][0-9]+)'
aborts_in_logging_regex = r'Aborts in logging: *([0-9]+)'
aborts_in_validating_regex = r'Aborts in validating: *([0-9]+)'
aborts_in_redo_regex = r'Aborts in redo: *([0-9]+)'
aborts_in_sgl_regex = r'Aborts in SGL: *([0-9]+)'
alloc_highmark_regex = r'Alloc high mark: *([0-9]+)'
free_highmark_regex = r'Free high mark: *([0-9]+)'
allocs_logged_regex = r'Allocs logged: *([0-9]+)'
allocs_reallocated_regex = r'Allocs reallocated: *([0-9]+)'
writes_per_commit_regex = r'.*Writes per commit: *([0-9]+[.][0-9]+)'
valid_fail_mismatch_regex = r'Crafty validation failures due to write mismatches: *([0-9]+)'
valid_fail_toofew_regex = r'.*Crafty validation failures due to too few writes: *([0-9]+)'
valid_fail_regex = r'Crafty validation failures: *([0-9]+)'
write_per_tx_hist_regex = r'{} *: *([0-9]+)'
redo_success_regex = r'Applying redo log succeeded:  *([0-9]+)'
validate_success_regex = r'Validating log succeeded: *([0-9]+)'
readonly_success_regex = r'Readonly tx succeeded: *([0-9]+)'
memory_regex = r'Maximum resident set size: ([0-9]+) KB'
sgl_single_write_regex = r'Crafty single write SGL sections: *([0-9]+)'
sgl_multi_write_regex = r'Crafty multi  write SGL sections: *([0-9]+)'
total_writes_regex = r'Total writes: *([0-9]+)'
redo_failures_regex = r'Crafty replay failures: *([0-9]+)'
sgl_inner_aborts_regex = r'SGL inner aborts due to {}: *([0-9]+)'
redo_aborts_regex = r'redo aborts due to {}: *([0-9]+)'
memcached_throughput_regex = r'Run time: *[0-9.]+s Ops: [0-9]+ TPS: ([0-9]+) Net_rate: [0-9.]+./s'

categories_regex = r'([a-zA-Z-]+)[.]([a-zA-Z0-9-]+)[.]([0-9]+)thr[.]run[0-9]+[.]log$'

writes_per_tx = {}
exp, runs, (time, commits, fallbacks, conflicts, capacity, explicit, zero,
            aborts, aborts_in_logging, aborts_in_validating, aborts_in_redo, aborts_in_sgl, abort_percentage,
            alloc_highmark, free_highmark, allocs_logged, allocs_reallocated, writes_per_commit,
            valid_fail, valid_fail_mistmatch, valid_fail_toofew,
            writes_per_tx['0'], writes_per_tx['1'], writes_per_tx['2'], writes_per_tx['3'], writes_per_tx['4-5'], writes_per_tx['6-10'], writes_per_tx['11+'],
            redo_success, validate_success, readonly_success, memory, sgl_single_write, sgl_multi_write, total_writes, redo_failures,
            sgl_inner_aborts_conflicts, sgl_inner_aborts_capacity, sgl_inner_aborts_explicit, sgl_inner_aborts_zero,
            redo_aborts_conflicts, redo_aborts_capacity, redo_aborts_explicit, redo_aborts_zero, memcached_throughput) = exparser.collect_regex_mean([
    (time_regex, float, categories_regex),
    (commits_regex, int, categories_regex),
    (fallbacks_regex, int, categories_regex),
    (conflicts_regex, int, categories_regex),
    (capacity_regex, int, categories_regex),
    (explicit_regex, int, categories_regex),
    (zero_regex, int, categories_regex),
    (aborts_regex, int, categories_regex),
    (aborts_in_logging_regex, int, categories_regex),
    (aborts_in_validating_regex, int, categories_regex),
    (aborts_in_redo_regex, int, categories_regex),
    (aborts_in_sgl_regex, int, categories_regex),
    (abort_percentage_regex, float, categories_regex),
    (alloc_highmark_regex, int, categories_regex),
    (free_highmark_regex, int, categories_regex),
    (allocs_logged_regex, int, categories_regex),
    (allocs_reallocated_regex, int, categories_regex),
    (writes_per_commit_regex, float, categories_regex),
    (valid_fail_regex, int, categories_regex),
    (valid_fail_mismatch_regex, int, categories_regex),
    (valid_fail_toofew_regex, int, categories_regex),
    (write_per_tx_hist_regex.format('0'), int, categories_regex),
    (write_per_tx_hist_regex.format('1'), int, categories_regex),
    (write_per_tx_hist_regex.format('2'), int, categories_regex),
    (write_per_tx_hist_regex.format('3'), int, categories_regex),
    (write_per_tx_hist_regex.format('4-5'), int, categories_regex),
    (write_per_tx_hist_regex.format('6-10'), int, categories_regex),
    (write_per_tx_hist_regex.format('11\\+'), int, categories_regex),
    (redo_success_regex, int, categories_regex),
    (validate_success_regex, int, categories_regex),
    (readonly_success_regex, int, categories_regex),
    (memory_regex, int, categories_regex),
    (sgl_single_write_regex, int, categories_regex),
    (sgl_multi_write_regex, int, categories_regex),
    (total_writes_regex, int, categories_regex),
    (redo_failures_regex, int, categories_regex),
    (sgl_inner_aborts_regex.format('conflicts'), int, categories_regex),
    (sgl_inner_aborts_regex.format('capacity'), int, categories_regex),
    (sgl_inner_aborts_regex.format('explicit'), int, categories_regex),
    (sgl_inner_aborts_regex.format('zero'), int, categories_regex),
    (redo_aborts_regex.format('conflicts'), int, categories_regex),
    (redo_aborts_regex.format('capacity'), int, categories_regex),
    (redo_aborts_regex.format('explicit'), int, categories_regex),
    (redo_aborts_regex.format('zero'), int, categories_regex),
    (memcached_throughput_regex, int, categories_regex),
],
    '', # exp name
    '', # basename
    '', # config
    INPUT_PATH,
    flatlogs=True
)


def show(p):
    if SHOW_GRAPHS:
        try:
            p.show()
        except Exception:
            pass
    p.close()


solution_color = {
    "HTM-only": "xkcd:sky blue",
    "DudeTM": "xkcd:light orange",
    "NV-HTM": "xkcd:apple green",
    "Crafty": "xkcd:crimson",
    "Crafty-NoRedo": "xkcd:hunter green",
    "Crafty-NoValidate": "xkcd:blue violet",
}

solution_line = {
    "HTM-only": "-",
    "DudeTM": "--",
    "NV-HTM": "-.",
    "Crafty": "-",
    "Crafty-NoRedo": "--",
    "Crafty-NoValidate": "-.",
}

solution_name = {
    "HTM-only": "Non-durable",
    "DudeTM": "DudeTM",
    "NV-HTM": "NV-HTM",
    "Crafty": "Crafty",
    "Crafty-NoRedo": "Crafty-NoRedo",
    "Crafty-NoValidate": "Crafty-NoValidate",
}

benchmark_name = {
     'bayes': 'bayes',
     'genome': 'genome',
     'intruder': 'intruder',
     'kmeans-high': 'kmeans-high',
     'kmeans-low': 'kmeans-low',
     'labyrinth': 'labyrinth',
     'ssca2': 'ssca2',
     'vacation-high': 'vacation-high',
     'vacation-low': 'vacation-low',
     'yada': 'yada',
     'bank-fee': 'bank-medium',
     'bank-fee-hc': 'bank-high',
     'bank-fee-nc': 'bank-none',
     'bplustree': 'bplustree',
     'bplustree-wronly': 'bplustree-wronly',
     'memcached': 'memcached',
}

TOP_CUTOFF_MARGIN = 2


with (OUTPUT_PATH / 'results.tex').open('w') as _output_file:
    
    def fprint(*args, **kwargs):
        print(*args, **kwargs, file=_output_file if PRINT_TO_FILE else sys.stdout)
    
    def fnprint(*args, **kwargs):
        fprint(*args, **kwargs, end='')
    
    # Writes per commit
    fprint("""
    \\begin{table}
    \\footnotesize
    \\begin{tabular}{@{}l|rrrrrrr@{}}
    Solution & 1 & 2 & 4 & 8 & 12 & 15 & 16 \\\\
    \\hline
    """)
    for benchmark in benchmarks:
        fnprint(benchmark)
        for solution in ['Crafty']:
            for thread_count in map(str, THREAD_COUNTS):
                try:
                    _wri = total_writes.results[solution][benchmark][thread_count]['mean']
                    _val = validate_success.results[solution][benchmark][thread_count]['mean']
                    _sgl = fallbacks.results[solution][benchmark][thread_count]['mean']
                    _red = redo_success.results[solution][benchmark][thread_count]['mean']
                    fnprint(' & {:.1f} '.format(_wri / (_val + _sgl + _red)))
                except TypeError as e:
                    fnprint(' & - ')
            fprint('\\\\')
    fprint("""
    \\end{tabular}
    \\caption{Number of writes per persistent transaction on average.
    \\label{tab:write-per-ptx}}
    \\end{table}
    """)

    def across_threads(results_, solution_, benchmark_):
        means = []
        for thread_count_ in map(str, THREAD_COUNTS):
            try:
                means.append(results_.results[solution_][benchmark_][thread_count_]['mean'])
            except TypeError:
                means.append(0)
        return np.array(means)

    # Aborts graph
    N = len(THREAD_COUNTS)
    ind = np.arange(0, N * 2, 2)
    width = 0.20
    bar_sep = 1.2
    linewidth = 0.2
    for benchmark in benchmarks:
        plt.figure(figsize=[12.8, 7.2])
        for i, solution in enumerate(solutions):
            offset = width * (i - (len(solutions) - 1) / 2.0) * bar_sep
            conflict_means = across_threads(conflicts, solution, benchmark) + across_threads(redo_aborts_conflicts, solution, benchmark) + across_threads(sgl_inner_aborts_conflicts, solution, benchmark)
            capacity_means = across_threads(capacity, solution, benchmark) + across_threads(redo_aborts_capacity, solution, benchmark) + across_threads(sgl_inner_aborts_capacity, solution, benchmark)
            explicit_means = across_threads(explicit, solution, benchmark) + across_threads(redo_aborts_explicit, solution, benchmark) + across_threads(sgl_inner_aborts_explicit, solution, benchmark)
            zero_means = across_threads(zero, solution, benchmark) + across_threads(redo_aborts_zero, solution, benchmark) + across_threads(sgl_inner_aborts_zero, solution, benchmark)
            total_means = across_threads(aborts, solution, benchmark)
            commit_means = across_threads(commits, solution, benchmark) \
                           + across_threads(redo_success, solution, benchmark) \
                           + across_threads(sgl_multi_write, solution, benchmark)


            def __plot(means, _bottom, color, hatch):
                _p = plt.bar(ind + offset, means, width, color=color, linewidth=linewidth, edgecolor="xkcd:black", hatch=hatch, bottom=_bottom)
                if _bottom is None:
                    _bottom = means
                else:
                    _bottom += means
                return _p, _bottom

            pcomm, bottom = __plot(commit_means, None, 'xkcd:really light blue', '')
            pconf, bottom = __plot(conflict_means, bottom, 'xkcd:sky blue', '')
            pcapa, bottom = __plot(capacity_means, bottom, 'xkcd:eggshell', '//')
            pexpl, bottom = __plot(explicit_means, bottom, 'xkcd:leaf green', '\\')
            pzero, bottom = __plot(zero_means, bottom, 'xkcd:crimson', '')

            if all((i > 1 for i in bottom)):
                for rect in pzero.patches:
                    text = plt.annotate(
                        solution_name[solution],
                        xy=(rect.get_x() + rect.get_width() / 2, 0),
                        xytext=(0, 5),
                        textcoords='offset points',
                        va='bottom',
                        ha='center',
                        rotation=90,
                        color='black',
                    )
                    #text.set_path_effects([path_effects.Stroke(linewidth=2, foreground='black'), path_effects.Normal()])
        plt.ylabel('Hardware transactions')
        plt.xlabel('Thread count')
        plt.xticks(ind, map(str, THREAD_COUNTS))
        yaxis = plt.gca().get_yaxis()
        formatter = ticker.ScalarFormatter()
        formatter.set_scientific(False)
        yaxis.set_major_formatter(formatter)
        plt.legend((pcomm[0], pconf[0], pcapa[0], pexpl[0], pzero[0]), ('Commit', 'Conflict', 'Capacity', 'Explicit', 'Zero'))
        plt.savefig(str(OUTPUT_PATH / ('conflicts-' + benchmark + '.eps')), format='eps', dpi=1200)
        show(plt)

    # Successful persistent transaction breakdown
    N = len(THREAD_COUNTS)
    ind = np.arange(0, N * 2, 2)
    width = 0.20
    bar_sep = 1.2
    linewidth = 0.2
    for benchmark in benchmarks:
        plt.figure(figsize=[12.8, 7.2])
        for i, solution in enumerate(solutions):
            offset = width * (i - (len(solutions) - 1) / 2.0) * bar_sep
            commit_means = across_threads(commits, solution, benchmark)
            validation_means = across_threads(validate_success, solution, benchmark)
            sgl_means = across_threads(fallbacks, solution, benchmark)
            redo_means = across_threads(redo_success, solution, benchmark)
            readonly_means = across_threads(readonly_success, solution, benchmark)

            if 'Crafty' in solution:
                pread = plt.bar(ind + offset, readonly_means, width, color='xkcd:eggshell',
                                linewidth=linewidth, edgecolor='xkcd:black')
                predo = plt.bar(ind + offset, redo_means, width, color='xkcd:pale blue',
                            linewidth=linewidth, edgecolor='xkcd:black', hatch='//', bottom=readonly_means)
                pvalid = plt.bar(ind + offset, validation_means, width, bottom=redo_means+readonly_means, color='xkcd:apple green', linewidth=linewidth,
                            edgecolor='xkcd:black')
                bottom = readonly_means + redo_means + validation_means
            else:
                pcommit = plt.bar(ind + offset, commit_means, width, color='xkcd:baby blue',
                            linewidth=linewidth, edgecolor='xkcd:black', hatch='\\')
                bottom = commit_means
            psgl = plt.bar(ind + offset, sgl_means, width, bottom=bottom,
                            color='xkcd:scarlet', linewidth=linewidth, edgecolor='xkcd:black', hatch='\\\\')

            if all((i > 1 for i in (bottom + sgl_means))):
                for rect in psgl.patches:
                    text = plt.annotate(
                        solution_name[solution],
                        xy=(rect.get_x() + rect.get_width() / 2, 0),
                        xytext=(0, 5),
                        textcoords='offset points',
                        va='bottom',
                        ha='center',
                        rotation=90,
                        color='black',
                    )
                    #text.set_path_effects([path_effects.Stroke(linewidth=2, foreground='black'), path_effects.Normal()])
        plt.ylabel('Persistent transactions')
        plt.xlabel('Thread count')
        plt.xticks(ind, THREAD_COUNTS)
        yaxis = plt.gca().get_yaxis()
        formatter = ticker.ScalarFormatter()
        formatter.set_scientific(False)
        yaxis.set_major_formatter(formatter)
        plt.legend((pcommit[0], pread[0], predo[0], pvalid[0], psgl[0]), ('Non-Crafty', 'Read Only', 'Redo', 'Validate', 'SGL'))
        plt.savefig(str(OUTPUT_PATH / ('persistent-tx-' + benchmark + '.eps')), format='eps', dpi=1200)
        show(plt)


    x = np.array(THREAD_COUNTS)
    # Throughput
    N = len(THREAD_COUNTS)
    ind = np.arange(0, N * 2, 2)
    width = 0.20
    bar_sep = 1.2
    linewidth = 0.2
    for benchmark in benchmarks:
        max_single = 0
        baseline = (memcached_throughput if benchmark == 'memcached' else time).results['HTM-only'][benchmark]['1']['mean']
        if benchmark in ['bank-fee', 'bank-fee-nc', 'bank-fee-hc']:
            plt.figure(figsize=[6.4, 4.5])
        else:
            plt.figure(figsize=[6.4, 3.6])
        for i, solution in enumerate(solutions):
            if benchmark == 'memcached':
                ptx_p_s = across_threads(memcached_throughput, solution, benchmark) / baseline
                plt.plot(x, ptx_p_s,
                         label=solution, color=solution_color[solution], linestyle=solution_line[solution])
                max_single = max(np.max(ptx_p_s), max_single)
            else:
                offset = width * (i - (len(solutions) - 1) / 2.0) * bar_sep

                throughput_means = []
                throughput_errs = []
                bench_results = time.results[solution]
                for thread_count in map(str, THREAD_COUNTS):
                    try:
                        vals = bench_results[benchmark][thread_count]['data']
                        throughput = 1 / (np.array(vals) / baseline)
                        mean = np.mean(throughput)
                        conf = ss.t.interval(0.95, len(throughput) - 1, loc=mean, scale=ss.sem(throughput))
                        conf_diff = mean - conf[0]
                        throughput_means.append(mean)
                        throughput_errs.append(conf_diff)
                    except TypeError as e:
                        throughput_means.append(0)
                        throughput_errs.append(0)
                if any((i < 0.0001 for i in throughput_means)):
                    continue
                r = plt.errorbar(x, throughput_means, yerr=throughput_errs, label=solution_name[solution], color=solution_color[solution], linestyle=solution_line[solution],
                                 markersize=4, capsize=5)  # fmt=o,s,*,p,D
                max_single = max(np.max(throughput_means), max_single)
        plt.ylabel('Normalized throughput')
        plt.xlabel('Thread count')
        plt.xticks(np.arange(1, max(THREAD_COUNTS) + 1, 1.0))
        for i, label in enumerate(plt.gca().get_xticklabels()):
            if i + 1 not in THREAD_COUNTS:
                label.set_visible(False)
        plt.ylim(bottom=0, top=max_single*1.1)
        if benchmark not in ['bplustree']:
            plt.legend()
        plt.savefig(str(OUTPUT_PATH / ('throughput-' + benchmark + '.eps')), format='eps', dpi=1200)
        show(plt)

