#!/bin/sh
echo Exp2
./s02.rerun_1st_exp.sh
cp cde-package/cde-root/home/quanpt/Tools/cde/mytest/bash/out.from.exp1.txt ./in.for.exp2.txt
md5sum in.for.exp2.txt
echo Exp2.done
