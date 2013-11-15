#!/bin/sh
cd cde-package
~/Tools/cde/scripts/prov2dot.py -f provenance.cde-root.1.log --withgraph
cat provenance.cde-root-pkg02.1.log provenance.cde-root.2.log > prov.log
~/Tools/cde/scripts/prov2dot.py -f prov.log -d gv2
~/Tools/cde/scripts/prov2dot.py -f provenance.cde-root.2.log -d gv3 --withgraph
