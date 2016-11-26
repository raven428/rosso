#!/bin/dash -e
# decorate
br=$(mktemp "$LOCALAPPDATA"/temp/XXX)

for ch in *.c *.h
do
  printf '#include "%s"\n' "$br" >> "$ch"
done

# transform
make -k INCLUDE=1 2>&1 |
awk '
/should add these lines|has correct/ {
  $0 = "\33[1;32m" $0
}
/should remove these lines/ {
  $0 = "\33[1;31m" $0
}
/full include-list for/ {
  $0 = "\33[1;33m" $0
}
/make|^$|---/ {
  $0 = "\33[m" $0
}
1
'

# undecorate
for ch in *.c *.h
do
  ex -sc 'd|x' "$ch"
done
