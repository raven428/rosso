#!/bin/dash -e
if [ "$#" != 1 ]
then  
  cat <<+
SYNOPSIS
  input.sh [drive]

EXAMPLE
  input.sh F:
+
  exit
fi

./rosso -i "$1"
