#!/bin/sh
cd cde-package
if [ ! -d gv ]
then
  ~/Tools/cde/scripts/prov2dot.py -f provenance.cde-root.1.log --withgraph
  cat provenance.cde-root-pkg02.1.log provenance.cde-root.2.log > prov.log
  ~/Tools/cde/scripts/prov2dot.py -f prov.log -d gv2
  ~/Tools/cde/scripts/prov2dot.py -f provenance.cde-root.2.log -d gv3 --withgraph
fi
~/Tools/cde/scripts/graphcmp.py gv/main.graph gv3/main.graph
