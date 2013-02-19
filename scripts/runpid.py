#!/usr/bin/python

import sys
import os
import re
import argparse

parser = argparse.ArgumentParser(description='Find and execute the node specified by pid and cmd.')
parser.add_argument('-f', action="store", dest="file_name", default="provenance.log")
parser.add_argument('-d', action="store", dest="cde_package", default=".")
parser.add_argument('-p', action="store", dest="pid", help="pid of a node", required=True, type=int)
parser.add_argument('-c', action="store", dest="cmd", help="partial string of node's command", default="")

args = parser.parse_args()
logfile = args.file_name
package = args.cde_package
pid_param = str(args.pid)
cmd_param = args.cmd

def run_prog(pwd, cmd, par):
  os.chdir(package)
  scriptname = cmd_param + '.' + pid_param + '.cde'
  fout = open(scriptname, 'w')
  fout.write("""#!/bin/sh
HERE="$(dirname "$(readlink -f "${0}")")"
""" +
'cd "$HERE/cde-root' + pwd + '" && $HERE/cde-exec \'' + cmd + '\' "$@"')
  fout.close()
  
  os.system('chmod u+x ' + scriptname)
  os.system('pwd && echo ./' + scriptname + ' ' + par)
  os.system('./' + scriptname + ' ' + par)

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
  action = words[2]
  
  if action == 'EXECVE': # found the issuer
    if words[3] == pid_param and re.match('.*' + cmd_param + '.*', words[4]):
      path = words[4]
      pwd = words[5]
      param = words[6]
  
  elif action == 'EXECVE2': # found the issuer success report
    if words[1] == pid_param:
      param = re.sub(r'^\["[^,]*", "', '["', param)
      param = re.sub(r'^\["', '', param)
      param = re.sub(r'"]$', '', param)
      param = param.replace('", "', ' ')
      
      fin.close()
      run_prog(pwd, path, param)
      sys.exit(0)

fin.close()
sys.exit(-1)
