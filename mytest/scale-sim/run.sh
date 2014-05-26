#!/bin/sh

for l in `cat  cslist.txt`
do
  ssh $l kill -9 -1
done

rm -rf cde-package
rm -rf ~/cde-package

time -p ../../ptu $@ ./exp.sh

### post-process db for indirect (spawn) links
../../scripts/db2dot.py -f cde-package/provenance.cde-root.1.log -d gv1 > /dev/null 2>&1

cd cde-package
for l in provenance.cde-root.1.log.*_db
do
  ../../scripts/db2dot.py -f $l -d gv > /dev/null 2>&1
done
