import numpy as np
import scipy.stats as ss


def mean(results, confidence=0.95):
    for bench, stats in results.items():
        if 'data' not in stats:
            for k, v in stats.items():
                mean(v, confidence=confidence)
        else:
            values = np.array(stats['data'])
            #values = reject_outliers(values) # Outliers shouldn't exist under normal conditions
            stats['mean'] = np.mean(values)
            stats['med'] = np.median(values)
            stats['confidence'] = ss.t.interval(confidence, len(values) - 1, loc=stats['mean'], scale=ss.sem(values))
            stats['confidence-diff'] = stats['mean'] - stats['confidence'][0]


def stdev(results):
    for bench, stats in results.items():
        if 'data' not in stats:
            for k, v in stats.items():
                stdev(v)
        else:
            stats['stdev'] = np.std(np.array(stats['data']))


def max(results):
    for bench, stats in results.items():
        if 'data' not in stats:
            for k, v in stats.items():
                max(v)
        else:
            stats['max'] = np.max(np.array(stats['data']))


# https://stackoverflow.com/a/16562028
def reject_outliers(data, m=3.5):
    d = np.abs(data - np.median(data))
    mdev = np.median(d)
    s = d / mdev if mdev else 0.
    return data[s < m]
