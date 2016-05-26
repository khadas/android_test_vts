#!/usr/bin/env python

from setuptools import setup
from setuptools import find_packages
import sys

install_requires = [
    'future',
    'futures',
    'concurrent',
]

if sys.version_info < (3,):
    install_requires.append('enum34')

setup(
    name='vts',
    version='0.1',
    description='Android Vendor Test Suite',
    license='Apache2.0',
    packages=find_packages(),
    include_package_data=False,
    install_requires=install_requires,
    url="http://www.android.com/"
)
