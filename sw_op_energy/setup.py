#!/usr/bin/env python
try:
    from setuptools import setup
except ImportError:
    from distutils.core import setup

setup(name='sw_op_energy',
      version='0.a.0',
      description='Python OP_ENERGY Tools',
      author='dbb',
      author_email='dbb@test.com',
      url='http://github.com/vbut/pybitcointools',
      packages=['oetool'],
      scripts=['oe2'],
      include_package_data=True,
      data_files=[("", ["LICENSE"]), ("oetool", ["oetool/english.txt"])],
      )



