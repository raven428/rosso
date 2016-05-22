#!/bin/sed -f
#-*-mode:perl-*-
1,3 ! d
s/^/#define /
s/=//
