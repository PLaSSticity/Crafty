from pathlib import Path
from exparser.collections import Run


def runs(experiment, basedir=(Path.home() / 'exp-output'), filter_benches=lambda x: x):
    expdir = Path(basedir) / experiment.basename
    assert (expdir / 'rerun').exists(), "Directory {} is not an exp output directory".format(expdir)
    benches = filter(Path.is_dir, expdir.iterdir())
    rs = []
    for bench in filter(filter_benches, benches):
        runsdir = bench / 'adapt' / 'gc_default' / experiment.config / 'var'
        runs = filter(Path.is_dir, runsdir.iterdir())
        for run in runs:
            output = run / 'output.txt'
            assert output.exists(), "Experiment {}'s run in {} is missing it's output".format(
                experiment.name, run
            )
            rs.append(Run(bench.parts[-1], output))
    return rs


def runs_flat(experiment, basedir=(Path.home() / 'Crafty-logs')):
    rs = []
    for run in filter(Path.is_file, basedir.iterdir()):
        rs.append(Run(None, run))
    return rs
