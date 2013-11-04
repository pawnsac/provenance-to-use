#!/bin/sh
echo Exp1 Result:
echo "1st exp" > data.1exp.txt
cat data.1exp.txt
cp data.1exp.txt out1.txt
date >> out1.txt
sleep 1
echo "done 1st exp"
cat out1.txt
pwd
echo "======="

