#!/bin/sh
cd cde-package
if [ ! -d gv ]
then
  ../../../scripts/prov2dot.py -f provenance.cde-root-Y6*.1.log --withgraph
  cat provenance.cde-root-YP*.1.log  provenance.cde-root-Y6*.2.log > prov.log
  ../../../scripts/prov2dot.py -f prov.log -d gv2
  ../../../scripts/prov2dot.py -f provenance.cde-root-Y6*.2.log -d gv3 --withgraph
fi
#../../../scripts/graphcmp.py gv/main.graph gv3/main.graph
dot -Tpng gv/main.gv -o exp1.png
dot -Tpng gv2/main.gv -o exp2.png
mv exp*.png ~/html/
