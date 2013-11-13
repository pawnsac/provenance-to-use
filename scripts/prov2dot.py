#!/usr/bin/python

# dot -Tsvg -o out.svg in.gv
# digraph cdeprov2dot {
# graph [rankdir = "RL"];
# node [fontname="Helvetica" fontsize="8" style="filled" margin="0.0,0.0"];
# edge [fontname="Helvetica" fontsize="8"];
# "nnnn1" [label="nnnn1" URL="url1" shape="box" fillcolor="lightsteelblue1"]
# "nnnn2" [label="nnnn2" URL="url2" shape="box" fillcolor="lightsteelblue1"]
# "nnnn1" -> "nnnn2" [label="" color="blue"]
# }

import re
from subprocess import call
import os, sys, time, datetime
import glob
import argparse

def isFilteredPath(path):
  if re.match('\/proc\/', path) is None \
    and re.match('.*\/lib\/', path) is None \
    and re.match('\/etc\/', path) is None \
    and re.match('\/var\/', path) is None \
    and re.match('\/dev\/', path) is None \
    and re.match('\/sys\/', path) is None \
    and re.match('.*\/R\/x86_64-pc-linux-gnu-library\/', path) is None \
    and re.match('.*\/usr\/share\/', path) is None:
    return False
  else: 
    return True

parser = argparse.ArgumentParser(description='Process provenance log file.')
parser.add_argument('--nosub', action="store_true", default=False)
parser.add_argument('--nofilter', action="store_true", default=False)
parser.add_argument('--withfork', action="store_true", default=False)
parser.add_argument('-f', action="store", dest="fin_name", default="provenance.log")
parser.add_argument('-d', action="store", dest="dir_name", default="./gv")

args = parser.parse_args()

showsub = not args.nosub
filter = not args.nofilter
withfork = args.withfork
dir = args.dir_name
logfile = args.fin_name

# open input output files
try:
  fin = open(logfile, 'r')
except IOError:
  print "Error: can\'t find file " + logfile + " or read data\n"
  sys.exit(-1)

# prepare graphic directory
if not os.path.exists(dir):
  os.makedirs(dir)
os.system("rm -f " + dir + "/*.gnu " + dir + "/*.svg " + dir + "/*.gv " + dir + "/*.html")

fout = open(dir + '/main.gv', 'w')
fout.write("""digraph cdeprov2dot {
graph [rankdir = "RL" ];
node [fontname="Helvetica" fontsize="8" style="filled" margin="0.0,0.0"];
edge [fontname="Helvetica" fontsize="8"];
"cdenet" [label="CDENet" shape="box" fillcolor="blue"];\n""")
f2out = open(dir + '/main.process.gv', 'w')
f2out.write("""digraph cdeprovshort2dot {
graph [rankdir = "RL" ];
node [fontname="Helvetica" fontsize="8" style="filled" margin="0.0,0.0"];
edge [fontname="Helvetica" fontsize="8"];
"cdenet" [label="CDENet" shape="box" fillcolor="blue"];
"unknown" [label="unknown" shape="box" fillcolor="lightsteelblue1"];
""")
fhtml = open(dir + '/main.html', 'w')
fhtml.write("""<h1>Overview</h1>
<a href=main.svg>Full Graph</a><br/>
<a href=main.process.svg>Process Graph</a><br/>
""")
fhtml.close()

active_pid = {'0':'unknown'}
cde_pid = -1
pid_desc = {'cdenet':'[label="CDENet" shape="box" fillcolor="blue" URL="main.process.svg"]',
  'unknown':'[label="unknown" shape="box" fillcolor="blue" URL="main.process.svg"]'}
pid_starttime = {}
pid_mem = {}
pid_graph = {}
counter = 1

for line in fin:
  if re.match('^#.*$', line) or re.match('^$', line):
    continue
  line = line.rstrip('\n')
  words = line.split(' ', 4)
  pid = words[1]
  action = words[2]
  path = '' if len(words) < 4 else words[3]
  path = path.replace('"', '\"')
  
  if pid in active_pid:
    node = active_pid[pid] # possible NULL
  else:
    cde_pid = pid
    active_pid[pid] = 'cdenet'
  
  if action == 'EXECVE': # this case only, node is the child words[3]
    nodename = words[3] + '_' + str(counter)
    node = '"' + nodename + '"'
    label = time.ctime(int(words[0])) + \
        '\\n PID: ' + words[3] + "\\n" + ''.join(words[4:]).replace('\\', '\\\\').replace('"','\\"') \
        .replace(', \\"',', \\n\\"').replace('[\\"','\\n[\\"')
    counter += 1
    active_pid[words[3]] = node # store the dict from pid to unique node name
    
    # main graph
    pid_desc[node] = ' [label="' + label + '" shape="box" fillcolor="lightsteelblue1" URL="' + nodename + '.prov.svg"]'
    fout.write(node + pid_desc[node] + '\n')
    fout.write(active_pid[pid] + ' -> ' + node + ' [label="" color="darkblue"]\n')
    
    # main process graph
    #pid_desc[node] = ' [label="' + label + '" shape="box" fillcolor="lightsteelblue1" URL="' + nodename + '.prov.svg"]'
    f2out.write(node + pid_desc[node] + '\n')
    f2out.write(active_pid[pid] + ' -> ' + node + ' [label="" color="darkblue"]\n')
    
    # html file of this pid
#     htmlf = open('gv/' + nodename + '.html', 'w')
#     htmlf.write('Memory Footprint: <img src=' + nodename + '.mem.svg /><br/>' +
#       'Provenance Graph: <object data=' + nodename + '.prov.svg type="image/svg+xml"></object>')
#     htmlf.close()

    if showsub:
      # mem graph of this pid
      pid_mem[node] = open(dir + '/' + nodename + '.mem.gnu', 'w')
      pid_mem[node].write('set terminal svg\nset output "' + nodename + '.mem.svg"\n' +
        'set xlabel "Time (seconds) from ' + time.ctime(int(words[0])) + '\n' +
        'set ylabel "Memory Usage (kB)"\n' +
        'set xrange [-1:]\n' +
        'plot "-" title "' + label + '" with lines\n' +
        '0\t0\n')
      pid_starttime[node] = int(words[0])
      
      parentnode = active_pid[pid]
      # prov graph of this pid
      pid_graph[node] = open(dir + '/' + nodename + '.prov.gv', 'w')
      pid_graph[node].write('digraph "' + nodename + '" {\n'+
        """graph [rankdir = "RL" ];
        node [fontname="Helvetica" fontsize="8" style="filled" margin="0.0,0.0"];
        edge [fontname="Helvetica" fontsize="8"];
        """ +
        'Memory [label="" shape=box image="' + nodename + '.mem.svg"];\n' +
        node + ' [label="' + label + '" shape="box" fillcolor="green"]\n' +
        parentnode + pid_desc[parentnode] + '\n' +
        parentnode + ' -> ' + node + ' [label="" color="darkblue"]\n')
        
      # prov graph of parent pid
      try:
        pid_graph[parentnode].write(
          node + pid_desc[node] + '\n' +
          parentnode + ' -> ' + node + ' [label="" color="darkblue"]\n')
      except:
        pass
    
  elif action == 'SPAWN':
    node = '"' + words[3] + '_' + str(counter) + '"'
    label = 'fork ' + words[3]
    counter += 1
    parentnode = active_pid[pid]
    active_pid[words[3]]=node # store the dict from pid to unique node name
    pid_desc[node] = pid_desc[parentnode]
    
    if withfork:
      # main graph
      fout.write(node + ' [label="' + label + '" shape="box" fillcolor="azure"];\n')
      fout.write(parentnode + ' -> ' + node + ' [label="" color="darkblue"];\n')
      
      # main process graph
      f2out.write(node + ' [label="' + label + '" shape="box" fillcolor="azure"];\n')
      f2out.write(parentnode + ' -> ' + node + ' [label="" color="darkblue"];\n')
    else:
      active_pid[words[3]]=active_pid[pid]

    
  elif action == 'EXIT':
    if showsub:
      try:
        pid_mem[node].close()
        del pid_mem[node]
        pid_graph[node].write("}")
        pid_graph[node].close()
        del pid_graph[node]
      except:
        pass
      del active_pid[pid]
    
  elif action == 'READ':
    if (not filter or not isFilteredPath(path)): 
      fout.write('"' + path + '" -> ' + node + ' [label="" color="blue"];\n')
      if showsub:
        try:# TOFIX: what about spawn node
          pid_graph[node].write('"' + path + '" -> ' + node + ' [label="" color="blue"];\n')
        except:
          pass
  elif action == 'WRITE':
    if (not filter or not isFilteredPath(path)): 
      fout.write(node + ' -> "' + path + '" [label="" color="blue"];\n')
      if showsub:
        try:
          pid_graph[node].write(node + ' -> "' + path + '" [label="" color="blue"];\n')
        except:
          pass
  elif action == 'READ-WRITE':
    if (not filter or not isFilteredPath(path)): 
      fout.write(node + ' -> "' + path + '" [dir="both" label="" color="blue"];\n')
      if showsub:
        try:
          pid_graph[node].write(node + ' -> "' + path + '" [dir="both" label="" color="blue"];\n')
        except:
          pass
    
  elif action == 'MEM':
    if showsub:
      rel_time = int(words[0]) - pid_starttime[node]
      pid_mem[node].write(str(rel_time) + '\t' + str(int(words[3])>>10) + '\n')
    
fout.write("}")
fout.close()
f2out.write("}")
f2out.close()
fin.close()

if showsub:
  for node in pid_graph:
    try:
      pid_graph[node].write("}")
      pid_graph[node].close()
    except:
      pass

  for node in pid_mem:
    try:
      pid_mem[node].close()
    except:
      pass

def removeMultiEdge(filename):
  lines = [line for line in open(filename)]
  newlines = lines[:5]
  newlines = newlines + list(set(lines[5:-1])) + ['}']
  f = open(filename, 'w')
  for line in newlines:
    f.write(line)
  f.close()

# covert created graphviz and gnuplot files into svg files
os.chdir(dir)
os.system("dot -Tsvg main.process.gv -o main.process.svg")
print("Processing main.gv, this might take a long time ...\n")
removeMultiEdge("main.gv")
os.system("dot -Tsvg main.gv -o main.svg")
os.system("gnuplot *.gnu")
for fname in glob.glob("./*.prov.gv"):
  removeMultiEdge(fname)
  os.system("dot -Tsvg " + fname + " -o " + fname.replace('prov.gv', 'prov.svg'))
os.system('echo "<h1>Processes</h1>" >> main.html')
os.system('ls *.prov.svg | while read l; do echo "<a href=$l>$l</a><br/>" >> main.html; done')
os.system('echo "<h1>Memory Footprints</h1>" >> main.html')
os.system('ls *.mem.svg | while read l; do echo "<a href=$l>$l</a><br/>" >> main.html; done')
