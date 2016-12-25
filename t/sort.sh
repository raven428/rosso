#!/bin/dash -e
xc() {
  awk 'BEGIN {d = "\47"; printf "\33[36m"; while (++j < ARGC) {
  k = split(ARGV[j], q, d); q[1]; for (x in q) printf "%s%s",
  q[x] ~ /^[[:alnum:]%+,./:=@_-]+$/ ? q[x] : d q[x] d, x < k ? "\\" d : ""
  printf j == ARGC - 1 ? "\33[m\n" : FS}}' "$@"
  "$@"
}

if [ "$#" != 1 ]
then  
  cat <<'q'
SYNOPSIS
  sort.sh [drive]

EXAMPLE
  sort.sh F:

NOTES
  All files will be removed from drive
q
  exit
fi

j=$1
find "$j"/ -mindepth 1 -maxdepth 1 -exec rm -r {} +

for g in alfa bravo charlie delta
do
  mkdir "$j/$g"
  for k in 1 2 3 4
  do
    touch "$j/$g/$k".txt
  done
done

xc ./rosso -R "$j"
xc ./rosso "$j"
xc ./rosso -l "$j"

xc ./rosso -R "$j"
xc ./rosso -d / "$j"
xc ./rosso -l "$j"
xc ./rosso -l -d / "$j"

xc ./rosso -R "$j"
xc ./rosso -d / -d CHARLIE F:
xc ./rosso -l "$j"
xc ./rosso -l -d / -d CHARLIE "$j"
