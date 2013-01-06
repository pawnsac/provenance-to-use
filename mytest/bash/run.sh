#!/bin/sh
pwd
echo 1234 > /dev/null
sleep 1 0 0 0 0 string &
ps
slept 1
cat hello.txt
echo "here" > output.txt
sleep 1 &
ps | grep sleep
echo "done"
