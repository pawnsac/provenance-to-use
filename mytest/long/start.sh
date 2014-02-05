#!/bin/bash
n=$1
if [ $n = 10 ]
then
#  telnet localhost 2222
  wget "http://people.cs.uchicago.edu/~quanpt/spim.pdf"
  exit
else
  n=`expr $n + 1`
  echo $n
  ./start.sh $n
fi
