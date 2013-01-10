#!/bin/sh
cd javart
../cde java TestExec
#cat provenance.log | grep -v READ
mkdir -p gv
../prov2dot.py
