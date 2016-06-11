#!/bin/dash -e
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
rm -f "$j"/*

for k in 1 2 3 4 5
do
  touch "$j/$k".txt
done

./rosso -R "$j"
./rosso -l "$j"
./rosso "$j"
./rosso -l "$j"
