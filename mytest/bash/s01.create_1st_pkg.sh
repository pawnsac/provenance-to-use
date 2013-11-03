#!/bin/sh
rm -rf cde-package
rm -f provenance.*log
../../../wincde/ptu sh myexp.sh
cd cde-package
mv cde.log exp.sh
cd ..
find cde-package > pkg01.txt
