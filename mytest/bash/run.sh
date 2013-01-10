#!/bin/sh
pwd
echo 1234 > /dev/null
sleep 4 0 0 0 0 string &
ps
slept 4
cat hello.txt
echo "here" > output.txt
sleep 2 &
ps | grep sleep
sleep 5
echo "done"
