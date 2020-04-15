import exparser
from exparser import Experiment, discover, Collector


def collect_regex_mean(regex_and_parser_and_categories, expname, basename, config, basedir, findstats=True, flatlogs=False):
    exp = Experiment(expname, basename=basename, config=config)
    discover_runs = discover.runs if not flatlogs else discover.runs_flat
    runs = discover_runs(exp, basedir=basedir)
    colls = [Collector(r, p, c) for r, p, c in regex_and_parser_and_categories]
    exparser.collect(runs, colls)
    if findstats:
        for coll in colls:
            exparser.mean(coll.results, confidence=0.95)
            exparser.stdev(coll.results)
            exparser.max(coll.results)
    return exp, runs, colls
