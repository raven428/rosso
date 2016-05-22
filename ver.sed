#!/bin/sed -f
1,3 ! d
s/^/#define /
s/=//
s/[[:digit:]]/"&"/
