#!/bin/bash
bin=$(dirname $0)/../svm
ret=0
for f in $(dirname $0)/*/*.in; do
  output=$($bin < $f)
  expected=$(cat ${f%.in}.out)
  if [[ X$output != X$expected ]]; then
    echo Test failed!
    echo $f
    cat $f
    echo Expected: $expected
    echo Output: $output
    echo
    ret=1
  fi
done
exit $ret
