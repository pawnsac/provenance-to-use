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
import os, sys, time, datetime, glob, json
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
parser.add_argument('--withgraph', action="store_true", default=False)

args = parser.parse_args()

showsub = not args.nosub
filter = not args.nofilter
withfork = args.withfork
dir = args.dir_name
logfile = args.fin_name
withgraph = args.withgraph

meta = {}
colors=['pink','yellow','lightgreen','lightblue'] 
colorid=0
# BIG TODO: colors are currently arranged to make the later overcolors the former
#    so that sub-graph overcolors the parent graph nodes
re_set = {}

def processMetaData(line):
  m = re.match('# @(\w+): (.*)$', line)
  (key, value) = m.group(1, 2)
  meta[key] = value
  if key == "namespace":
    re_set['rmns'] = re.compile('"[^"]*' + meta['namespace'] + '\/')
  return key

def makePathNode(path):
  filename=os.path.basename(path).replace('"','\\"')
  node=path.replace('\\', '\\\\').replace('"','\\"')
  nodedef='"' + node + '"[label="' + filename + '", tooltip="' + node + '", shape="", fillcolor=' + colors[colorid] + ']'
  return (node, nodedef)
  
def getEdgeStr(node, pathnode, deptype):
  if deptype == "wasGeneratedBy":
    return '"' + pathnode + '" -> ' + node
  elif deptype == "used" or deptype == "rw":
    return node + ' -> "' + pathnode + '"'
  else:
    return 'ERROR -> ERROR'

def printArtifactDep(node, path, deptype, filter):
  if (not filter or not isFilteredPath(path)):
    direction = 'dir="both" ' if deptype == "rw" else ''
    (pnode, pdef) = makePathNode(path)
    edge = getEdgeStr(node, pnode, deptype)
    fout.write(pdef + '\n')
    fout.write(edge + ' [' + direction +'label="' + deptype + '" color=""];\n') # blue
    if showsub:
      try:# TOFIX: what about spawn node
        pid_graph[node].write(pdef + '\n')
        pid_graph[node].write(edge + ' [' + direction +'label="' + deptype + '" color=""];\n') # blue
      except:
        pass
    if withgraph:
      fgraph.write(re_set['rmns'].sub('"/', edge) + '\n')

# open input output files
try:
  fin = open(logfile, 'r')
except IOError:
  print "Error: can\'t find file " + logfile + " or read data\n"
  sys.exit(-1)

while 1:
  line = fin.readline()
  if re.match('^#.*$', line) or re.match('^$', line):
    if re.match('^# @.*$', line):
      processMetaData(line)
    continue
  else:
    break
  

# prepare graphic directory
if not os.path.exists(dir):
  os.makedirs(dir)
os.system("rm -f " + dir + "/*.gnu " + dir + "/*.svg " + dir + "/*.gv " + dir + "/*.html")

fout = open(dir + '/main.gv', 'w')
fout.write("""digraph G {
graph [rankdir = "RL" ];
node [fontname="Helvetica" fontsize="8" style="filled" margin="0.0,0.0"];
edge [fontname="Helvetica" fontsize="8"];
"cdenet" [label="XXX" shape="box" fillcolor=""" +colors[colorid]+ "];\n" + \
"\"namespace" + meta['fullns'] + '"[shape=box label="' + meta['agent'] + "@" + meta['fullns'] + '" color=' +colors[colorid]+ ']')
f2out = open(dir + '/main.process.gv', 'w')
f2out.write("""digraph cdeprovshort2dot {
graph [rankdir = "RL" ];
node [fontname="Helvetica" fontsize="8" style="filled" margin="0.0,0.0"];
edge [fontname="Helvetica" fontsize="8"];
"cdenet" [label="XXX" shape="box" fillcolor=""];
"unknown" [label="unknown" shape="box" fillcolor=""];
""") # blue lightsteelblue1
fhtml = open(dir + '/main.html', 'w')
fhtml.write("""<h1>Overview</h1>
<a href=main.svg>Full Graph</a><br/>
<a href=main.process.svg>Process Graph</a><br/>
""")
fhtml.close()

if withgraph:
  fgraph = open(dir + '/main.graph', 'w')

active_pid = {'0':'unknown'}
info_pid = {}
cde_pid = -1
pid_desc = {'cdenet':'[label="XXX" shape="box" fillcolor="" URL="main.process.svg"]', # blue
  'unknown':'[label="unknown" shape="box" fillcolor="" URL="main.process.svg"]'} # blue
pid_starttime = {}
pid_mem = {}
pid_graph = {}
counter = 1

while 1:
  if re.match('^#.*$', line) or re.match('^$', line):
    if re.match('^# @.*$', line):
      if processMetaData(line) == 'fullns':
        colorid += 1
        fout.write('"namespace' + meta['fullns'] + '"[shape=box label="' + meta['agent'] + "@" + meta['fullns'] + '" color=' +colors[colorid]+ ']\n')
    line = fin.readline()
    continue
  line = line.rstrip('\n').replace('\\', '\\\\').replace('"','\\"')
  words = line.split(' ', 6)
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
    label = 'PID: ' + words[3] + "\\n" + os.path.basename(words[4])
    title = time.ctime(int(words[0])) + ' ' + words[4] + ' ' + ''.join(words[5:])
    #    .replace(', \\"',', \\n\\"').replace('[\\"','\\n[\\"')
    counter += 1
    active_pid[words[3]] = node # store the dict from pid to unique node name
    info_pid[words[3]] = {'path': words[4], 'dir': words[5]}
    
    # main graph
    pid_desc[node] = ' [label="' + label + '" tooltip="' + title + '" shape="box" fillcolor="' + colors[colorid] + '" URL="' + nodename + '.prov.svg"]' # lightsteelblue1
    fout.write(node + pid_desc[node] + '\n')
    fout.write(node + ' -> ' + active_pid[pid] + ' [label="wasTriggeredBy" color=""]\n') # darkblue
    
    if withgraph:
      fgraph.write(node + ' : ' + json.dumps(info_pid[words[3]]) + '\n')
      fgraph.write(node + ' -> ' + active_pid[pid] + '\n')
    
    # main process graph
    f2out.write(node + pid_desc[node] + '\n')
    f2out.write(node + ' -> ' + active_pid[pid] + ' [label="wasTriggeredBy" color=""]\n') # darkblue
    
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
        node + ' [label="' + label + '" shape="box" fillcolor=""]\n' + #green
        parentnode + pid_desc[parentnode] + '\n' +
        node + ' -> ' + parentnode + ' [label="wasTriggeredBy" color=""]\n') #darkblue
        
      # prov graph of parent pid
      try:
        pid_graph[parentnode].write(
          node + pid_desc[node] + '\n' +
          node + ' -> ' + parentnode + ' [label="wasTriggeredBy" color=""]\n') #darkblue
      except:
        pass
    
  elif action == 'SPAWN':
    node = '"' + words[3] + '_' + str(counter) + '"'
    label = 'fork ' + words[3]
    counter += 1
    parentnode = active_pid[pid]
    active_pid[words[3]]=node # store the dict from pid to unique node name
    pid_desc[node] = pid_desc[parentnode]
    
    if withfork: # not handle the withgraph case
      # main graph
      fout.write(node + ' [label="' + label + '" shape="box" fillcolor=""];\n') #azure
      fout.write(node + ' -> ' + parentnode + ' [label="wasTriggeredBy" color=""];\n') #darkblue
      
      # main process graph
      f2out.write(node + ' [label="' + label + '" shape="box" fillcolor=""];\n') #azure
      f2out.write(node + ' -> ' + parentnode + ' [label="wasTriggeredBy" color=""];\n') #darkblue
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
    pinfo = info_pid[words[1]]
    if os.path.abspath(pinfo['path']) != path:
      printArtifactDep(node, path, "used", filter)
  elif action == 'WRITE':
    printArtifactDep(node, path, "wasGeneratedBy", filter)
  elif action == 'READ-WRITE':
    printArtifactDep(node, path, "rw", filter)
    
  elif action == 'MEM':
    if showsub:
      rel_time = int(words[0]) - pid_starttime[node]
      pid_mem[node].write(str(rel_time) + '\t' + str(int(words[3])>>10) + '\n')
  
  line = fin.readline()
  if line == '':
    break
  
fout.write("}")
fout.close()
if withgraph:
  fgraph.close()
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
#os.system("unflatten -f -l3 main.gv | dot -Tsvg -Edir=none -Gsplines=ortho -o main.svg")
os.system("gnuplot *.gnu")
for fname in glob.glob("./*.prov.gv"):
  removeMultiEdge(fname)
  os.system("dot -Tsvg " + fname + " -o " + fname.replace('prov.gv', 'prov.svg'))
os.system('echo "<h1>Processes</h1>" >> main.html')
os.system('ls *.prov.svg | while read l; do echo "<a href=$l>$l</a><br/>" >> main.html; done')
os.system('echo "<h1>Memory Footprints</h1>" >> main.html')
os.system('ls *.mem.svg | while read l; do echo "<a href=$l>$l</a><br/>" >> main.html; done')
