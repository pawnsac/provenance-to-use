#!/bin/sh

rm -rf output.txt output barrier.txt

touch barrier.txt

~/assi/cde/mytest/scale-sim/split.py

for l in `cat cslist.ln.txt`
do
  ~/assi/cde/mytest/scale-sim/single.sh $l &
done

# wait for every single jobs done
TARGETN=`cat cslist.ln.txt | wc -l`
while b=`cat barrier.txt | wc -l`; test $b -ne $TARGETN; 
do 
  sleep 1
done

~/assi/cde/mytest/scale-sim/aggregate.py

wc output.txt
