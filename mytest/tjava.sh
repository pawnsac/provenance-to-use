#!/bin/sh
cd javart
./cde java TestExec
cat provenance.log | grep -v READ

