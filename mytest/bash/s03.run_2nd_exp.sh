#!/bin/sh
./s02.run_1st_exp.sh
echo "this is 2nd experiment data" > data.2exp.txt
cp data.2exp.txt temp.txt
echo "Result: " > out.txt
cat temp.txt >> out.txt
echo "===" >> out.txt
cat out.txt
