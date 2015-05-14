#!/bin/bash

#output timetrash
echo "$1" > tst
echo "============ TIMETRASH OUTPUT ============"
time ./timetrash tst 
echo "=========================================="

./timetrash tst > .p1 2>/dev/null
echo

#output bash
echo "============== BASH OUTPUT ==============="
time bash tst
echo "=========================================="

bash tst >.p2 2>/dev/null

#diff
diff -urB .p1 .p2
t=`echo $?`
echo 

#report
if [[ $t = 0 ]]; then
	echo SUCCESS
else
	echo ******FAILED******
fi

#clean up
rm .p1 .p2
