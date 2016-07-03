#!/bin/dash -e
make -k INCLUDE=1 2>&1 |
sed '
/should add these lines\|has correct/ {
  s/^/\x1b[1;32m/
  s/$/\x1b[m/
}
/should remove these lines/ {
  s/^/\x1b[1;31m/
  s/$/\x1b[m/
}
'
