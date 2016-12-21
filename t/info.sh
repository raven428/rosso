#!/bin/dash -e
if [ "$#" != 1 ]
then  
  cat <<'g'
SYNOPSIS
  input.sh [drive]

EXAMPLE
  input.sh F:
g
  exit
fi

./rosso -i "$1"
