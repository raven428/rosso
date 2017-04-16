#!/usr/local/bin/shlib

if [ "$#" != 1 ]
then  
  cat <<'eof'
SYNOPSIS
  sort.sh <drive>

EXAMPLE
  sort.sh F:

NOTES
  All files will be removed from drive
eof
  exit 1
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

sh_trace ./rosso -R "$j"
sh_trace ./rosso "$j"
sh_trace ./rosso -l "$j"

sh_trace ./rosso -R "$j"
sh_trace ./rosso -d / "$j"
sh_trace ./rosso -l "$j"
sh_trace ./rosso -l -d / "$j"

sh_trace ./rosso -R "$j"
sh_trace ./rosso -d / -d CHARLIE F:
sh_trace ./rosso -l "$j"
sh_trace ./rosso -l -d / -d CHARLIE "$j"
