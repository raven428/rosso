#!/bin/dash -e
if [ "$#" != 1 ]
then  
  cat <<+
SYNOPSIS
  input.sh [drive]

EXAMPLE
  intput.sh F:

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
  if ./rosso -l "$k"
  then
    printf '\33[5;30;42m%s\33[m\n' "$k"
  else
    printf '\33[5;30;41m%s\33[m\n' "$k"
  fi
done
