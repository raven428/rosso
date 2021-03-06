#!/bin/dash -e
if [ "$#" != 1 ]
then  
  cat <<'eof'
SYNOPSIS
  input.sh <drive>

EXAMPLE
  input.sh F:

NOTES
  All files will be removed from drive
eof
  exit 1
fi

j=$1
find "$j"/ -mindepth 1 -maxdepth 1 -exec rm -r {} +

for k in 1 2 3 4 5
do
  touch "$j/$k".txt
done

for k in '\\.\'"$j"'\' '\\.\'"$j" "$j"'\' "$j"
do
  if ./rosso -l "$k"
  then
    printf '\33[5;30;42m%s\33[m\n' "$k"
  else
    printf '\33[5;30;41m%s\33[m\n' "$k"
  fi
done
