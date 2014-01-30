#!/bin/sh

### prepare clean db and newest binary
rm -rf cde-package/provenance.cde-root.*
cp ../../ptu cde-package/cde-exec

### start server
../../ptu ./s > /dev/null 2>&1 &
#../../ptu ./s &
#./s &
sleep 0.5

### start client
#echo "msg1 msg2 hello world msg5 msg6" | ../../ptu ./c > /dev/null 2>&1
echo "msg1 msg2 hello world msg5 msg6" | ../../ptu ./c
sleep 0.5

### exit server
/bin/echo -ne "_exit" | telnet localhost 8888
sleep 0.5

### post-process db for indirect (spawn) links
../../scripts/db2dot.py -f cde-package/provenance.cde-root.1.log -d gv1
../../scripts/db2dot.py -f cde-package/provenance.cde-root.2.log -d gv2
