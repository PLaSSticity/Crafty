from collections import namedtuple


Experiment = namedtuple('Experiment', [
        'name',
        'basename',
        'config',
])


Run = namedtuple('Run', ['bench', 'path'])
