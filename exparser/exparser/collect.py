import re
from collections import defaultdict


def _ensuredict(dict, key):
    if isinstance(dict[key], list):
        dict[key] = defaultdict(list)



class Collector:
    def __init__(self, pattern, parser=float, categories=''):
        self.__matcher = re.compile(pattern)
        self.__categories = re.compile(categories) if categories != '' else None
        self.results = Results()
        self.__parser = parser
        self._collect_count = 0
        self._collect_match_count = 0

    def collect(self, run, line):
        match = self.__matcher.match(line)
        if match is not None:
            self._collect_match_count += 1
            if len(match.groups()) == 1:
                if self.__categories is None:
                    self.results[run.bench]['data'].append(self.__parser(match.groups()[0]))
                    self._collect_count += 1
                else:
                    d = self.results
                    for category in self.__categories.match(run.path.name).groups():
                        _ensuredict(d, category)
                        d = d[category]
                    d['data'].append(self.__parser(match.groups()[0]))
                    self._collect_count += 1
            else:
                self.results[run.bench]['data'].append(match.groups())
                self._collect_count += 1


class Results(defaultdict):
    def __init__(self):
        super().__init__(lambda: defaultdict(list))

    def getall(self, key):
        vals = []
        for inner in self.values():
            if key in inner:
                vals.append(inner[key])
        return vals


def collect(runs, collectors):
    for run in runs:
        with run.path.open() as file:
            for line in file:
                for collector in collectors:
                    collector.collect(run, line)
