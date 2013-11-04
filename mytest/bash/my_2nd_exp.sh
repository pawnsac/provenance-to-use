#!/bin/sh
./s02.rerun_1st_exp.sh
echo "Exp2 Result: "
#cp cde-package/cde-root/home/quanpt/assi/cde/mytest/bash/data.1exp.txt ./
echo "this is 2nd experiment data" > data.2exp.txt
cp data.2exp.txt temp.txt
cat temp.txt > out.txt
echo "===" >> out.txt
cat out.txt
ls *.txt
pwd
