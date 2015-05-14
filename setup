#!/bin/sh 
arg=`echo $#`
if [[ $arg > 1 || $arg = 1 && $1 != "-u" ]]; then
	echo "Invalid option"
	exit 1
fi;
echo -n "Changing following files permission to"
if [[ $arg = 1 ]]; then
	echo " -rw-r--r--"
	chmod 644 *.sh *.bash check-dist
else
	echo " -rwxr--r--"
	chmod 744 *.sh *.bash check-dist
fi
ls -1 *.sh *.bash check-dist
