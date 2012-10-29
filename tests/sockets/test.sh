#!/bin/bash
PORT=$((RANDOM%1000+10000))
../../cde ./server $PORT &
echo Hello there! | ../../cde ./client localhost $PORT