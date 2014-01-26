#!/bin/sh
rm -rf cde-package/provenance.cde-root.*
../../ptu ./s > /dev/null 2>&1 &
#../../ptu ./s &
sleep 0.5
#echo "hello world counter 1 2 3 4 5" | ../../ptu ./c > /dev/null 2>&1
echo "hello world counter 1 2 3 4 5" | ../../ptu ./c
sleep 0.5
/bin/echo -ne "_exit" | telnet localhost 8888
#../../scripts/db2dot.py -f cde-package/provenance.cde-root.1.log
#../../scripts/db2dot.py -f cde-package/provenance.cde-root.2.log
