#!/bin/sh
cd bash
../cde ./run.sh 
#cat provenance.log | grep -v \/proc\/
mkdir -p gv
../prov2dot.py
cd gv
dot -Tsvg provenance.gv -o main.svg
gnuplot *.gnu
