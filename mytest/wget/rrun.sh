#!/bin/sh
cp ../../ptu-ex cde-package/cde-exec
rm -rf cde-package/provenance.cde-root.1.log_db.*

### rerun
#URL="http://128.135.164.139/~quanpt/spim.pdf_notexist"
URL="http://people.cs.uchicago.edu/~quanpt/spim.pdf_notexist"
cd cde-package/cde-root/home/quanpt/assi/cde/mytest/wget
rm -f spim.pdf_notexist
../../../../../../../cde-exec $@ -N provenance.cde-root.1.log_db wget $URL
