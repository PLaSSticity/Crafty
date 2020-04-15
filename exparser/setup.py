from distutils.core import setup

__version__ = '0.1.0'
DESCRIPTION = 'Collects experiment results.'
LONG_DESCRIPTION = 'Collects experiment results, calculates statistics, and draws graphs.'

setup(
    name='exparser',
    version=__version__,
    description=DESCRIPTION,
    long_description=LONG_DESCRIPTION,
    author='Kaan Genç',
    author_email='genc.5@osu.edu',
    license='GPLv3',
    packages=[
        'exparser',
    ],
)