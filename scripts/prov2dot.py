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
import os
import time
import glob

# prepare graphic directory
if not os.path.exists("./gv"):
  os.makedirs("./gv")
os.system("rm gv/*.gnu gv/*.svg gv/*.gv gv/*.html")

# open input output files
fin = open('provenance.log', 'r')
fout = open('gv/main.gv', 'w')
fout.write("""digraph cdeprov2dot {
graph [rankdir = "RL"];
node [fontname="Helvetica" fontsize="8" style="filled" margin="0.0,0.0"];
edge [fontname="Helvetica" fontsize="8"];
"cdenet" [label="CDENet" shape="box" fillcolor="blue"];\n""")
f2out = open('gv/main.process.gv', 'w')
f2out.write("""digraph cdeprovshort2dot {
graph [rankdir = "RL"];
node [fontname="Helvetica" fontsize="8" style="filled" margin="0.0,0.0"];
edge [fontname="Helvetica" fontsize="8"];
"cdenet" [label="CDENet" shape="box" fillcolor="blue"];\n""")
fhtml = open('gv/main.html', 'w')
fhtml.write("""<h1>Overview</h1>
<a href=main.svg>Full Graph</a><br/>
<a href=main.process.svg>Process Graph</a><br/>
""")
fhtml.close()

active_pid = {'-1':'cdenet'}
pid_desc = {'cdenet':'[label="CDENet" shape="box" fillcolor="blue" URL="main.process.svg"]'}
pid_starttime = {}
pid_mem = {}
pid_graph = {}
counter = 1

for line in fin:
  line = line.rstrip('\n')
  words = line.split(' ', 4)
  pid = words[1]
  action = words[2]
  path = '' if len(words) < 4 else words[3]
  path = path.replace('"', '\"')
  node = active_pid[pid] # possible NULL
  
  if action == 'EXECVE': # this case only, node is the child words[3]
    nodename = words[3] + '_' + str(counter)
    node = '"' + nodename + '"'
    label = ' '.join(words[4:]).replace('\\', '\\\\').replace('"','\\"')
    counter += 1
    active_pid[words[3]]=node # store the dict from pid to unique node name
    
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

    # mem graph of this pid
    pid_mem[node] = open('gv/' + nodename + '.mem.gnu', 'w')
    pid_mem[node].write('set terminal svg\nset output "' + nodename + '.mem.svg"\n' +
      'set xlabel "Time (seconds) from ' + time.ctime(int(words[0])) + '\n' +
      'set ylabel "Memory Usage (kB)"\n' +
      'set xrange [-1:]\n' +
      'plot "-" title "' + label + '" with lines\n' +
      '0\t0\n')
    pid_starttime[node] = int(words[0])
    
    parentnode = active_pid[pid]
    # prov graph of this pid
    pid_graph[node] = open('gv/' + nodename + '.prov.gv', 'w')
    pid_graph[node].write('digraph "' + nodename + '" {\n'+
      """graph [rankdir = "RL"];
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
    
    # main graph
    fout.write(node + ' [label="' + label + '" shape="box" fillcolor="azure"];\n')
    fout.write(parentnode + ' -> ' + node + ' [label="" color="darkblue"];\n')
    
    # main process graph
    f2out.write(node + ' [label="' + label + '" shape="box" fillcolor="azure"];\n')
    f2out.write(parentnode + ' -> ' + node + ' [label="" color="darkblue"];\n')
    
  elif action == 'EXIT':
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
    if re.match('\/proc\/\d+\/stat.*', path) is None: # TODO: temporary remove ps big read
      fout.write('"' + path + '" -> ' + node + ' [label="" color="blue"];\n')
      try:# TOFIX: what about spawn node
        pid_graph[node].write('"' + path + '" -> ' + node + ' [label="" color="blue"];\n')
      except:
        pass
  elif action == 'WRITE':
    fout.write(node + ' -> "' + path + '" [label="" color="blue"];\n')
    try:
      pid_graph[node].write(node + ' -> "' + path + '" [label="" color="blue"];\n')
    except:
      pass
  elif action == 'READ-WRITE':
    fout.write(node + ' -> "' + path + '" [dir="both" label="" color="blue"];\n')
    try:
      pid_graph[node].write(node + ' -> "' + path + '" [dir="both" label="" color="blue"];\n')
    except:
      pass
    
  elif action == 'MEM':
    rel_time = int(words[0]) - pid_starttime[node]
    pid_mem[node].write(str(rel_time) + '\t' + str(int(words[3])>>10) + '\n')
    
fout.write("}")
fout.close()
f2out.write("}")
f2out.close()
fin.close()

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

# covert created graphviz and gnuplot files into svg files
os.chdir("./gv")
os.system("dot -Tsvg main.gv -o main.svg")
os.system("dot -Tsvg main.process.gv -o main.process.svg")
os.system("gnuplot *.gnu")
for fname in glob.glob("./*.prov.gv"):
  os.system("dot -Tsvg " + fname + " -o " + fname.replace('prov.gv', 'prov.svg'))
os.system('echo "<h1>Processes</h1>" >> main.html')
os.system('ls *.prov.svg | while read l; do echo "<a href=$l>$l</a><br/>" >> main.html; done')
os.system('echo "<h1>Memory Footprints</h1>" >> main.html')
os.system('ls *.mem.svg | while read l; do echo "<a href=$l>$l</a><br/>" >> main.html; done')