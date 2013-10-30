#!/bin/sh
cd /var/tmp
mkdir -p quanpt
cd quanpt
WORKDIR="hd.cde.$$"

mkdir -p $WORKDIR.datanode
cd $WORKDIR.datanode
/home/quanpt/assi/cde/ptu-ex /home/quanpt/hadoop/bin/hadoop-daemon.sh --config /home/quanpt/hadoop-1.1.2/libexec/../conf start datanode &
sleep 1

#cd ..
#mkdir -p $WORKDIR.tasktracker
#cd $WORKDIR.tasktracker
/home/quanpt/assi/cde/ptu-ex /home/quanpt/hadoop/bin/hadoop-daemon.sh --config /home/quanpt/hadoop-1.1.2/libexec/../conf start tasktracker &
