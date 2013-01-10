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

fin = open('provenance.log', 'r')
fout = open('gv/provenance.gv', 'w')
fout.write("""digraph cdeprov2dot {
graph [rankdir = "RL"];
node [fontname="Helvetica" fontsize="8" style="filled" margin="0.0,0.0"];
edge [fontname="Helvetica" fontsize="8"];
"cdenet" [label="CDENet" shape="box" fillcolor="blue"];\n""")

active_pid = {'-1':'cdenet'}
pid_mem = {}
counter = 1

for line in fin:
  line = line.rstrip('\n')
  words = line.split(' ', 4)
  pid = words[1]
  action = words[2]
  path = '' if len(words) < 4 else words[3]
  path = path.replace('"', '\"')
  
  if action == 'EXECVE':
    nodename = words[3] + '_' + str(counter)
    node = '"' + nodename + '"'
    label = ' '.join(words[4:]).replace('"','\\"')
    counter += 1
    active_pid[words[3]]=node # store the dict from pid to unique node name
    
    # main graph
    fout.write(node + ' [label="' + label + '" shape="box" fillcolor="lightsteelblue1" URL="' + nodename + '.svg"];\n')
    fout.write(active_pid[pid] + ' -> ' + node + ' [label="" color="darkblue"]\n')

    # prepare mem graph of this process
    pid_mem[node] = open('gv/' + nodename + '.mem.gnu', 'w')
    pid_mem[node].write('plot "-" title "' + label + '" with lines\n');
    
  elif action == 'SPAWN':
    node = '"' + words[3] + '_' + str(counter) + '"'
    label = 'fork ' + words[3]
    counter += 1
    active_pid[words[3]]=node # store the dict from pid to unique node name
    
    # main graph
    fout.write(node + ' [label="' + label + '" shape="box" fillcolor="azure"];\n')
    fout.write(active_pid[pid] + ' -> ' + node + ' [label="" color="darkblue"];\n')
    
  elif action == 'EXIT':
    node = active_pid[pid]
    pid_mem[node].close()
    del active_pid[pid]
    
  elif action == 'READ':
    if re.match('\/proc\/\d+\/stat.*', path) is None: # TODO: temporary remove ps big read
      fout.write('"' + path + '" -> ' + active_pid[pid] + ' [label="" color="blue"];\n')
  elif action == 'WRITE':
    fout.write(active_pid[pid] + ' -> "' + path + '" [label="" color="blue"];\n')
  elif action == 'READ-WRITE':
    fout.write(active_pid[pid] + ' -> "' + path + '" [dir="both" label="" color="blue"];\n')
    
  elif action == 'MEM':
    pid_mem[active_pid[pid]].write(words[0] + '\t' + words[3] + '\n')
    
fout.write("}")
fout.close()
fin.close()