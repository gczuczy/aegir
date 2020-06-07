#!/usr/bin/env python3

from setuptools import setup, find_packages

setup(
    name='aegir-api',
    version='0.1',
    packages=find_packages(where='lib'),
    package_dir={'': 'lib'},
    scripts=['bin/aegir-api', 'bin/aegir-api-wsgi'],

    install_requires=[],

    package_data= {
        },

    #metadata
    author='Gergely Czuczy',
    author_email='gergely.czuczy@harmless.hu',
    description='Aegir API middleware',
    keywords='beer HERMS RIMS',
    url='https://github.com/gczuczy/aegir'
    )
