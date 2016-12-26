#!/bin/dash -e
if [ "$#" != 1 ]
then  
  cat <<'eof'
SYNOPSIS
  input.sh [drive]

EXAMPLE
  input.sh F:
eof
  exit
fi

./rosso -i "$1"
