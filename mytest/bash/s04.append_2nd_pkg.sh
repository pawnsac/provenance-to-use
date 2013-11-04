#!/bin/sh
cp cde-package/cde-exec cde-package/cde
cde-package/cde -a cde-root-pkg02 ./s03.run_2nd_exp.sh
find cde-package > pkg02.txt
