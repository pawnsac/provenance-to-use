#!/bin/sh
/home/quanpt/hadoop/bin/hadoop-daemon.sh --config /home/quanpt/hadoop-1.1.2/libexec/../conf stop tasktracker
/home/quanpt/hadoop/bin/hadoop-daemon.sh --config /home/quanpt/hadoop-1.1.2/libexec/../conf stop datanode

