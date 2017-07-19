#!/usr/bin/env python2

import sys
import os
import re
import argparse
import logging
import time

def getFiles(pid_param, logfile):
  
  # monitoring pid
  pids = {}
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
      if pid in pids or words[3] == pid_param: 
        pids[words[3]] = 1
        path = words[4]
        pwd = words[5]
        param = words[6]
        paths.append(path)
        logging.debug(pid + "->" + words[3])
    
    elif action == 'SPAWN':
      if pid in pids:
        pids[words[3]] = 1
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
  return paths

def getProcs(pid_param, logfile):
  
  # monitoring pid
  pids = {}
  logpids = []
  
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
    
    if action == 'EXECVE': # found a monitored pid?
      if pid in pids or words[3] == pid_param: 
        pids[words[3]] = 1
        logpids.append(words[3])
        logging.debug(pid + "->" + words[3])
    
    elif action == 'EXECVE2': # found a child of a monitored pid?
      pass
    
    elif action == 'EXIT':
      if pid in pids:
        del pids[pid]
  
  fin.close()
  return logpids

def getProcsInRange(t1, t2):
  #time.ctime(int(words[0]))
  if t1 == "":
    t1 = 0
  else:
    t1 = ""

def makeTempPkg(file_list, package, temppkg):
  os.system('rm -rf "' + temppkg +'"')
  os.system('mkdir -p "' + temppkg +'"')
  for file in file_list:
    logging.debug('./okapi "' +file+ '" "' + package + '" "' + temppkg+'"')
    os.system('./okapi "' +file+ '" "' + package + '" "' + temppkg+'" 2>/dev/null')

if __name__ == "__main__":
  logging.basicConfig(stream=sys.stderr, level=logging.INFO)

  parser = argparse.ArgumentParser(description='Make dependency tree of the node specified by pid')
  parser.add_argument('-f', action="store", dest="file_name", default="provenance.log")
  parser.add_argument('-d', action="store", dest="cde_package", default=".")
  parser.add_argument('-t', action="store", dest="temp_package", default=".temp")
  parser.add_argument('-p', action="store", dest="pid", help="pid of a node", required=True, type=int)
  parser.add_argument('-c', action="store", dest="cmd", help="partial string of node's command", default="")
  parser.add_argument('-t1', action="store", dest="t1", default="")
  parser.add_argument('-t2', action="store", dest="t2", default="")
  
  # parsing parameters
  args = parser.parse_args()
  logfile = args.file_name
  package = args.cde_package
  pid_param = str(args.pid)
  temppkg = args.temp_package + "." + pid_param
  cmd_param = args.cmd
  t1 = args.t1
  t2 = args.t2
  
  print "List of files:"
  file_list = getFiles(pid_param, logfile)
  print file_list
  makeTempPkg(file_list, package + "/cde-root", temppkg)
  print "\n===\nList of process ids:"
  print getProcs(pid_param, logfile)
  print "\n===\nList of process ids in t1 - t2:"
  print getProcsInRange(t1, t2)
  
