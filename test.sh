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
  test.sh [drive]

EXAMPLE
  test.sh F:

NOTES
  All files will be removed from drive
+
  exit
fi

j=$1
rm -f "$j"/*

for k in 1 2 3 4 5
do
  touch "$j/$k".txt
done

for k in '\\.\'"$j"'\' '\\.\'"$j" "$j"'\' "$j"
do
  if xc ./rosso -R "$k"
  then
    ./rosso -l "$k"
    ./rosso "$k"
    ./rosso -l "$k"
  fi
done
