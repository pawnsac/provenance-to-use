#!/bin/sh
rm -rf cde-package
rm -f provenance.*log
../../../wincde/ptu sh my_1st_exp.sh
cd cde-package
mv cde.log mycopy_1st_exp.sh
cd ..
find cde-package > pkg01.txt
