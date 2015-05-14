#! /bin/sh

# UCLA CS 111 Lab 1 - Test that syntax errors are caught.

tmp=$0-$$.tmp
mkdir "$tmp" || exit
(
cd "$tmp" || exit
status=

n=1
for bad in \
  'lss' \
  'ls >' \
  'ls <' \
  'cat abc' \
  'cat < abc'\
  'ls -..'\
  'ls | cat a' 
do
  echo "$bad" >test$n.sh || exit
  ../timetrash  test$n.sh >test$n.out 2>test$n.err && {
    echo >&2 "test$n: unexpectedly succeeded for: $bad"
    status=1
  }
  test -s test$n.err || {
    echo >&2 "test$n: no error message for: $bad"
    status=1
  }
  n=$((n+1))
done

exit $status
) || exit

rm -fr "$tmp"
