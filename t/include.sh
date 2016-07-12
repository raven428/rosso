#!/bin/dash -e
# decorate
j=$(mktemp | cygpath -wf-)
for each in *.c *.h
do
  printf '#include "%s"\n' "$j" >> "$each"
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
git checkout *.c *.h
