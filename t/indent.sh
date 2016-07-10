#!/bin/dash
if [ $# != 1 ]
then
  echo 'indent.sh [file]'
  exit
fi
if ! type indent >/dev/null
then
  echo 'indent: not found'
  exit
fi
ex -s "$1" <<':'
" 1. decorate comments preceded by code
v,^\s*//,s,//,\r#&,
" 2. decorate printf format strings
g,\\n.$,s,$,//,
" 3. decorate bitwise AND
%s, &\_s, $\&\&,g
" 4. transform
%!indent -st
" 5. undecorate bitwise AND
%s,\v\$(\_s*)\&\& ?,\&\1,g
%s, $
" 6. undecorate printf format strings
%s, //$
" 7. undecorate comments preceded by code
%s,\n#\s\+, ,
x
:
