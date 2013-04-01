#!/usr/bin/python

import sys
import os
import re
import argparse
import checkpid

parser = argparse.ArgumentParser(description='Find and execute the node specified by pid and cmd.')
parser.add_argument('-f', action="store", dest="file_name", default="provenance.log")
parser.add_argument('-d', action="store", dest="cde_package", default=".")
parser.add_argument('-p', action="store", dest="pid", help="pid of a node", required=True, type=int)
parser.add_argument('-c', action="store", dest="cmd", help="partial string of node's command", default="")
parser.add_argument('-t', action="store", dest="temp_package", default=".temp")
parser.add_argument('-r', action="store", dest="replace_list", default="")

args = parser.parse_args()
logfile = args.file_name
package = args.cde_package
pid_param = str(args.pid)
cmd_param = args.cmd
temppkg = args.temp_package + "." + pid_param
rpl_list = args.replace_list

def run_prog(wd, cmd, par):
  os.chdir(package)
  
  # make temp package
  ###file_list = checkpid.getFiles(pid_param, logfile)
  ###checkpid.makeTempPkg(file_list, package + "/cde-root", temppkg + "/cde-root")
  print 'Running in "' + temppkg + '"'
  os.system('rm -rf "' + temppkg +'"')
  os.system('mkdir -p "' + temppkg +'"')
  os.system('cp -r . "' + temppkg + '" 2>/dev/null')
  os.system('cp cde-exec "' + temppkg + '"')
  os.system('cp cde.* "' + temppkg + '"')
  
  # replace list
  if rpl_list <> '':
    rpl_file = open(rpl_list, 'r')
    for line in rpl_file:
      if re.match('^#', line):
        continue  
      line = line.rstrip('\n')
      words = line.split('->', 2)
      os.system('cp -r "' + words[0] + '" "' + temppkg + '/cde-root' + words[1] +'"')
    rpl_file.close()
  
  # make script to run
  scriptname = cmd_param + '.' + pid_param + '.cde'
  fout = open(scriptname, 'w')
  fout.write("""#!/bin/sh
HERE="$(dirname "$(readlink -f "${0}")")"
""" +
'cd "$HERE/' + temppkg + '/cde-root' + wd + '" && $HERE/' + temppkg + '/cde-exec \'' + cmd + '\' "$@"')
  fout.close()
  
  # run it in new temp package
  os.system('chmod u+x ' + scriptname)
  #os.system('pwd && echo ./' + scriptname + ' ' + par)
  os.system('./' + scriptname + ' ' + par)

# open input output files
fin = open(logfile, 'r')

path = None
wd = None
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
      wd = words[5]
      param = words[6]
  
  elif action == 'EXECVE2': # found the issuer success report
    if words[1] == pid_param:
      param = re.sub(r'^\["[^,]*", "', '["', param)
      param = re.sub(r'^\["', '', param)
      param = re.sub(r'"]$', '', param)
      param = param.replace('", "', ' ')
      
      fin.close()
      run_prog(wd, path, param)
      sys.exit(0)

fin.close()
sys.exit(-1)
