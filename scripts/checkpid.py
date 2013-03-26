#!/usr/bin/python

import sys
import os
import re
import argparse
import logging

logging.basicConfig(stream=sys.stderr, level=logging.INFO)

parser = argparse.ArgumentParser(description='Make dependency tree of the node specified by pid')
parser.add_argument('-f', action="store", dest="file_name", default="provenance.log")
parser.add_argument('-d', action="store", dest="cde_package", default=".")
parser.add_argument('-t', action="store", dest="temp_package", default=".temp")
parser.add_argument('-p', action="store", dest="pid", help="pid of a node", required=True, type=int)
parser.add_argument('-c', action="store", dest="cmd", help="partial string of node's command", default="")

# parsing parameters
args = parser.parse_args()
logfile = args.file_name
package = args.cde_package
pid_param = str(args.pid)
temppkg = args.temp_package + "." + pid_param
cmd_param = args.cmd

# monitoring pid
pids = {}
pids[pid_param] = 1
paths = []

# open input output files
fin = open(logfile, 'r')

path = None
pwd = None
param = None

for line in fin:
  if re.match('^#', line):
    continue
  line = line.rstrip('\n')
  words = line.split(' ', 6)
  pid = words[1]
  action = words[2]
  path = '' if len(words) < 4 else words[3]
  
  if action == 'EXECVE': # found a monitored pid?
    if pid in pids:
      pids[words[3]] = 1
      path = words[4]
      pwd = words[5]
      param = words[6]
      paths.append(path)
      logging.debug(pid + "->" + words[3])
  
  elif action == 'EXECVE2': # found a child of a monitored pid?
    pass
  
  elif action == 'READ' or action == 'WRITE' or action == 'READ-WRITE':
    if pid in pids:
      paths.append(path)
      logging.debug(pid + "->" + path)
  
  elif action == 'EXIT':
    if pid in pids:
      del pids[pid]

fin.close()
print paths
