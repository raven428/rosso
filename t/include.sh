#!/bin/dash -e
# decorate
br=$(mktemp | cygpath -wf-)
for ch in *.c *.h
do
  printf '#include "%s"\n' "$br" >> "$ch"
done

# transform
make -k INCLUDE=1 2>&1 |
sed '
/should add these lines\|has correct/ {
  s/^/\x1b[1;32m/
}
/should remove these lines/ {
  s/^/\x1b[1;31m/
}
/full include-list for/ {
  s/^/\x1b[1;33m/
}
/make\|^$\|---/ {
  s/^/\x1b[m/
}
'

# undecorate
for ch in *.c *.h
do
  ex -sc 'd|x' "$ch"
done
