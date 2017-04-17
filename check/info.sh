#!/bin/dash -e
if [ "$#" != 1 ]
then  
  cat <<'eof'
SYNOPSIS
  input.sh <drive>

EXAMPLE
  input.sh F:
eof
  exit 1
fi

./rosso -i "$1"
