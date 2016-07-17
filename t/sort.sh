#!/bin/dash -e
xc() {
  awk '
  BEGIN {
    x = "\47"
    printf "\33[36m"
    while (++i < ARGC) {
      y = split(ARGV[i], z, x)
      for (j in z) {
        printf z[j] ~ /[^[:alnum:]%+,./:=@_-]/ ? x z[j] x : z[j]
        if (j < y) printf "\\" x
      }
      printf i == ARGC - 1 ? "\33[m\n" : FS
    }
  }
  ' "$@"
  "$@"
}

if [ "$#" != 1 ]
then  
  cat <<+
SYNOPSIS
  sort.sh [drive]

EXAMPLE
  sort.sh F:

NOTES
  All files will be removed from drive
+
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
