#!/usr/bin/env python

import sys,os

## usage:
##  oe_test_boost$  cp cmake-build-debug/oe_test_boost  .
##  oe_test_boost$ ./oe_test_boost  | ./rewrite_output_readme.py > README.md


outStream = sys.stdout
## write header
outStream.write( '###   cg_uint256' + "\n" )
outStream.write( '###    feb21  -dbb' + "\n" )
outStream.write( "\n\n" )

inStream = sys.stdin
##-- echo inputs
for line in inStream:
    outStream.write( '    ' + line)


