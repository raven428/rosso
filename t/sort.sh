#!/usr/local/bin/stdlib sh
# github.com/svnpenn/stdlib

if [ "$#" != 1 ]
then  
  cat <<'eof'
SYNOPSIS
  sort.sh [drive]

EXAMPLE
  sort.sh F:

NOTES
  All files will be removed from drive
eof
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

xtrace ./rosso -R "$j"
xtrace ./rosso "$j"
xtrace ./rosso -l "$j"

xtrace ./rosso -R "$j"
xtrace ./rosso -d / "$j"
xtrace ./rosso -l "$j"
xtrace ./rosso -l -d / "$j"

xtrace ./rosso -R "$j"
xtrace ./rosso -d / -d CHARLIE F:
xtrace ./rosso -l "$j"
xtrace ./rosso -l -d / -d CHARLIE "$j"
