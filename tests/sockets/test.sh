#!/bin/bash
PORT=$((RANDOM%1000+10000))
../../cde ./server $PORT &
echo Hello there! | ../../cde ./client localhost $PORT
# echo Hello there! | ../../cde ./client gabri.cs.uchicago.edu $PORT

echo -e "\n\n=== strace ==="
PORT=$((RANDOM%1000+10000))
../../cde ./server $PORT &
echo Hello there! | ../../cde ./client localhost $PORT
