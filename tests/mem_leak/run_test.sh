#!/bin/bash
RESTORE='\033[0m'

RED='\033[00;31m'
GREEN='\033[00;32m'
YELLOW='\033[00;33m'
BLUE='\033[00;34m'
PURPLE='\033[00;35m'
CYAN='\033[00;36m'
LIGHTGRAY='\033[00;37m'

LRED='\033[01;31m'
LGREEN='\033[01;32m'
LYELLOW='\033[01;33m'
LBLUE='\033[01;34m'
LPURPLE='\033[01;35m'
LCYAN='\033[01;36m'
WHITE='\033[01;37m'

rm report.txt; 

echo "==========================";
echo "   Memory-Leak Checker";
echo "==========================";
echo "Test for Bugs:";
echo "--------------------------";
for i in {1..1}; do 
	if (( $i < 10 ))
	then
		echo "Test0$i: " | tr -d '\n'; 
	else
		echo "Test$i: " | tr -d '\n'; 
	fi

	make SRC=test$i.c > /dev/null; 
	res=$(kminer -syscall=sys_test -leak -rm-deref -path-sens -report=report.txt test$i.bc | grep "bugs"); 
	echo $res | tr -d '\n';
	
	if [[ $res != *"0"* ]]
	then
		echo -e " $GREEN✓$RESTORE";
	else
		echo -e " $RED✗$RESTORE";
	fi
done 

#echo ""
#echo "==========================";
#echo "Test for False-Positives:";
#echo "--------------------------";
exit 1;
for i in {6..6}; do 
	if (( $i < 10 ))
	then
		echo "Test0$i: " | tr -d '\n'; 
	else
		echo "Test$i: " | tr -d '\n'; 
	fi

	make SRC=test$i.c > /dev/null; 
	res=$(kminer -syscall=sys_test -double-free -rm-deref -path-sens -report=report.txt test$i.bc | grep "bugs"); 
	echo $res | tr -d '\n';

	if [[ $res == *"0"* ]]
	then
		echo -e " $GREEN✓$RESTORE";
	else
		echo -e " $RED✗$RESTORE";
	fi
done 
