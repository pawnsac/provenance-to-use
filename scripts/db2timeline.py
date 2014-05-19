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

from subprocess import call
import re, os, sys, time, datetime, glob, json, argparse, logging as l, struct
from collections import deque
from datetime import datetime, timedelta
from leveldb import LevelDB, LevelDBError

epoch = datetime(1970, 1, 1)
timeline = []

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
colors=['yellow','pink','lightgreen','lightblue'] 
colorid=0
# BIG TODO: colors are currently arranged to make the later overcolors the former
#    so that sub-graph overcolors the parent graph nodes
re_set = {}
db = None
filelist = []

def main():

  global timeline
  
  # open db
  global db
  dberror = ''
  if os.path.exists(logfile+'_db'):
    try:
      db = LevelDB(logfile+'_db', create_if_missing = False)
    except LevelDBError:
      dberror = 'LevelDBError'
  else:
    if os.path.exists(logfile):
      try:
        db = LevelDB(logfile, create_if_missing = False)
      except LevelDBError:
        dberror = 'LevelDBError'
    else:
      dberror = 'os.path.exists ' + logfile

  if dberror != '':
    l.error(dberror)
    sys.exit(-1)
  
  # get root
  rootpid = db.Get('meta.root')
  fullns = ""
  agent = ""
  try:
    fullns = db.Get('meta.fullns')
    agent = db.Get('meta.agent')
  except:
    pass
  
  # prepare graphic directory
  if not os.path.exists(dir):
    os.makedirs(dir)
  os.system("rm -f " + dir + "/*.gnu " + dir + "/*.svg " + dir + "/*.gv " + dir + "/*.html")
  
  # prepare output file
  fout = open(dir + '/main.gv', 'w')
  fout.write("""digraph G {
  ranksep=.75;
  rankdir="LR";
  node [fontname="Helvetica" fontsize="8" style="filled" margin="0.0,0.0"];
  edge [fontname="Helvetica" fontsize="8" weight=1];
  """+
  rootpid + '[label="XXX" shape="box" fillcolor=' +colors[colorid]+ '];\n' + \
  "\"namespace:" + fullns + '"[shape=box label="' + agent + "@" + fullns + '" color=' +colors[colorid]+ ']\n')
  f2out = open(dir + '/main.process.gv', 'w')
  f2out.write("""digraph cdeprovshort2dot {
  graph [rankdir = "RL" ];
  node [fontname="Helvetica" fontsize="8" style="filled" margin="0.0,0.0"];
  edge [fontname="Helvetica" fontsize="8"];
  """+
  rootpid + '[label="XXX" shape="box" fillcolor=' +colors[colorid]+ '];\n' + \
  "\"namespace:" + fullns + '"[shape=box label="' + agent + "@" + fullns + '" color=' +colors[colorid]+ ']\n')
  fhtml = open(dir + '/main.html', 'w')
  fhtml.write("""<h1>Overview</h1>
  <a href=main.svg>Full Graph</a><br/>
  <a href=main.process.svg>Process Graph</a><br/>
  """)
  fhtml.close()
  if withgraph:
    fgraph = open(dir + '/main.graph', 'w')
  
  # make the graph
  pidqueue = deque([rootpid])
  db.Put('prv.pid.'+rootpid+'.actualpid', rootpid)
  while len(pidqueue) > 0:
    printGraph(pidqueue, fout, f2out)

  # make the timeline
  timeline = sorted(set(timeline))
  fout.write('{edge[weight=200]; node[group=timeline]; ')
  for t in timeline:
    fout.write('"'+str(t)+'"[shape=plaintext label="."];')
  fout.write('past -> ')
  for t in timeline:
    fout.write('"'+str(t)+'" -> ')
  fout.write('future;}')
  fout.write('{rank=same; "'+rootpid+'"; past;}')

  # done
  db.Put('meta.nomalized', '1')
  if withgraph:
    fgraph.close()
  fout.write("}")
  fout.close()
  f2out.write("}")
  f2out.close()
  db = None

  # covert created graphviz and gnuplot files into svg files
  os.chdir(dir)
  os.system("dot -Tsvg main.process.gv -o main.process.svg")
  print("Processing main.gv, this might take a long time ...\n")
  #removeMultiEdge("main.gv")
  os.system("dot -Tsvg main.gv -o main.svg")
  #os.system("unflatten -f -l3 main.gv | dot -Tsvg -Edir=none -Gsplines=ortho -o main.svg")
  os.system("gnuplot *.gnu")
  for fname in glob.glob("./*.prov.gv"):
    #removeMultiEdge(fname)
    os.system("dot -Tsvg " + fname + " -o " + fname.replace('prov.gv', 'prov.svg'))
  os.system('echo "<h1>Processes</h1>" >> main.html')
  os.system('ls *.prov.svg | while read l; do echo "<a href=$l>$l</a><br/>" >> main.html; done')
  os.system('echo "<h1>Memory Footprints</h1>" >> main.html')
  os.system('ls *.mem.svg | while read l; do echo "<a href=$l>$l</a><br/>" >> main.html; done')

def printGraph(pidqueue, f1, f2):
  pidkey = pidqueue.popleft()
  for (k, v) in db.RangeIter(key_from='prv.pid.'+pidkey+'.exec.', key_to='prv.pid.'+pidkey+'.exec.zzz'):
    try:
      db.Get('prv.pid.'+v+'.ok') # assert process successfully run
      
      # update actual pid (itself) and actual parent (parent.actualpid)
      db.Put('prv.pid.'+v+'.actualpid', v)
      parent = db.Get('prv.pid.'+pidkey+'.actualpid')
      db.Put('prv.pid.'+v+'.actualparent', parent)
      
      # update actual child
      # to replace: prv.pid.$(ppid.usec).exec.$usec -> $(pid.usec)
      time = k.split('.')[-1]
      db.Put('prv.pid.'+parent+'.actualexec.'+time, v)
      
      printProc(v, f1, f2)
      printExecEdge(v, f1, f2)
      pidqueue.append(v)
      
    except KeyError:
      print 'keyerror: pidkey %s k %s v %s' % (pidkey, k, v)
      pass
      
  for (k, v) in db.RangeIter(key_from='prv.pid.'+pidkey+'.spawn.', key_to='prv.pid.'+pidkey+'.spawn.zzz'):
    try:
      
      # don't print these spawn
      #printProc(v, f1, f2)
      #printExecEdge(pidkey, v, f1, f2)
      
      pidqueue.append(v)
      
      # update actual pid (parent.actualpid) and actual parent (parent.actualparent)
      db.Put('prv.pid.'+v+'.actualpid', db.Get('prv.pid.'+pidkey+'.actualpid'))
      db.Put('prv.pid.'+v+'.actualparent', db.Get('prv.pid.'+pidkey+'.actualparent'))
      
    except KeyError:
      print 'keyerror: prv.pid.%s.actualpid/actualparent.\n' % pidkey
      pass
      
  for (k, v) in db.RangeIter(key_from='prv.iopid.'+pidkey+'.', key_to='prv.iopid.'+pidkey+'.zzz'):
    try:
      if k[-3:] == '.fd' or k.startswith('prv.iopid.'+pidkey+'.actual'):
        continue
      # replace this: prv.iopid.$(pid.usec).$action.$usec -> $filepath
      actualpid = db.Get('prv.pid.'+pidkey+'.actualpid')
      (action, time) = k.split('.')[-2:]
      actiontime = '.'.join([action, time])
      db.Put('prv.iopid.'+actualpid+'.actual.'+actiontime, v)
      
      if filter and isFilteredPath(v):
        continue
      print k, v
      fnode = v.replace('\\', '\\\\').replace('"','\\"')
      closetime = long(time)
      try:
        fd = db.Get(k + '.fd')
        fd = str(struct.unpack('i', fd)[0])
        fnode = pidkey + '.' + time + '.' + fd
        closetime = db.Get('prv.file.'+pidkey+'.'+time+'.'+fd+'.close')
        closetime = struct.unpack('Q', closetime)[0]
      except KeyError:
        pass # this access doesn't return an fd
      printFileNode(fnode, v, time, closetime, f1)

      printFileEdge(pidkey, getFileAction(k), fnode, f1)
      
    except KeyError:
      print 'keyerror: prv.pid.%s.actualpid.\n' % pidkey
      pass

def getPidFromKey(pidkey):
  return pidkey.split('.')[0]
def getExecAttr(pidkey, attr):
  try:
    return db.Get('prv.pid.'+pidkey+'.'+attr)
  except KeyError:
    return None
def getExecStartTime(pidkey):
  try:
    return time.ctime((int)(db.Get('prv.pid.'+pidkey+'.start'))/1000000)
  except KeyError:
    return None
  except ValueError:
    print pidkey, db.Get('prv.pid.'+pidkey+'.start'), " ", + (int)(db.Get('prv.pid.'+pidkey+'.start'))/1000000
    return None
def hasKey(key):
  try:
    db.Get(key)
    return True
  except KeyError:
    return False
    
def printProc(pidkey, f1, f2):
  if hasKey('prv.pid.'+pidkey+'.ok'):
    label = 'PID: ' + getPidFromKey(pidkey) + "\\n" + os.path.basename(getExecAttr(pidkey, 'path'))
    title = getExecStartTime(pidkey) + ' ' + getExecAttr(pidkey, 'pwd') + ' ' + \
      getExecAttr(pidkey, 'args').replace('\\','\\\\  ').replace('"','\\"')
    line = pidkey + ' [label="' + label + '" tooltip="' + title + '" shape="box" fillcolor="' + colors[colorid] + '" URL="' + pidkey + '.prov.svg"]\n'
  else:
    line = pidkey + ' [shape="box"]\n'
  f1.write(line)
  f2.write(line)
  
def printExecEdge(pidkey, f1, f2):
  line = pidkey + ' -> ' + db.Get('prv.pid.'+pidkey+'.actualparent') + '[label="wasTriggeredBy"]\n'
  f1.write(line)
  f2.write(line)

def printFileNode(fnode, path, t1, t2, f1):
  filename=os.path.basename(path).replace('"','\\"')
  nodedef='"' + fnode + '"[label="' + filename + '", shape="", ' + \
      'fillcolor=' + colors[colorid] + ', tooltip="' + fnode + \
      ' ' + str(epoch + timedelta(microseconds=long(t1))) + \
      ' ' + str(epoch + timedelta(microseconds=t2)) + '" ]\n'
  f1.write(nodedef)
  #f2.write(nodedef)

  timeline.append(t2)
  if fnode[0] != '/':
    f1.write('{rank=same "'+str(t2)+'"; "'+fnode+'";}\n')
  
  
def printFileEdge(pidkey, action, path, f1):
  line = ''
  direction = ''
  pidkey = db.Get('prv.pid.'+pidkey+'.actualpid')
  if action == '1': 
    line = pidkey + ' -> "' + path + '" [label="used"];\n'
  elif action == '3':
    line = pidkey + ' -> "' + path + '" [dir="both" label="modified"];\n'
  elif action == '2':
    line = '"' + path + '" -> ' + pidkey + ' [label="wasGeneratedBy"];\n'
  else:
    line = '"' + path + '" -> ' + pidkey + ' [label="ERROR"];\n'
  f1.write(line)
  #f2.write(line)

def getFileAction(key):
  return key.split('.')[-2]

if __name__ == "__main__":
  main()
  sys.exit(-1)
else:
  sys.exit(-1)
  
  
### =======================

def processMetaData(line):
  m = re.match('# @(\w+): (.*)$', line)
  (key, value) = m.group(1, 2)
  meta[key] = value
  if key == "namespace":
    re_set['rmns'] = re.compile('"[^"]*' + meta['namespace'] + '\/')
  return key

  


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


while 1:
  line = fin.readline()
  if re.match('^#.*$', line) or re.match('^$', line):
    if re.match('^# @.*$', line):
      processMetaData(line)
    continue
  else:
    break
  


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
  newlines = lines[:4]
  newlines = newlines + list(set(lines[4:-1])) + ['}']
  f = open(filename, 'w')
  for line in newlines:
    f.write(line)
  f.close()

"""
digraph asde91 {
ranksep=.75; size = "7.5,7.5";
{
node [shape=plaintext, fontsize=16];
/* the time-line graph */
past -> 1978 -> 1980 -> 1982 -> 1983 -> 1985 -> 1986 ->
1987 -> 1988 -> 1989 -> 1990 -> "future";
/* ancestor programs */
"Bourne sh"; "make"; "SCCS"; "yacc"; "cron"; "Reiser cpp";
"Cshell"; "emacs"; "build"; "vi"; "<curses>"; "RCS"; "C*";
}
{ rank = same;
"Software IS"; "Configuration Mgt"; "Architecture & Libraries";
"Process";
};
node [shape=box];
{ rank = same; "past"; "SCCS"; "make"; "Bourne sh"; "yacc"; "cron"; }
{ rank = same; 1978; "Reiser cpp"; "Cshell"; }
{ rank = same; 1980; "build"; "emacs"; "vi"; }
{ rank = same; 1982; "RCS"; "<curses>"; "IMX"; "SYNED"; }
{ rank = same; 1983; "ksh"; "IFS"; "TTU"; }
{ rank == same; 1985; "nmake"; "Peggy"; }
{ rank = same; 1986; "C*"; "ncpp"; "ksh-i"; "<curses-i>"; "PG2"; }
{ rank = same; 1987; "Ansi cpp"; "nmake 2.0"; "3D File System"; "fdelta";
"DAG"; "CSAS";}
{ rank = same; 1988; "CIA"; "SBCS"; "ksh-88"; "PEGASUS/PML"; "PAX";
"backtalk"; }
{ rank = same; 1989; "CIA++"; "APP"; "SHIP"; "DataShare"; "ryacc";
"Mosaic"; }
{ rank = same; 1990; "libft"; "CoShell"; "DIA"; "IFS-i"; "kyacc"; "sfio";
"yeast"; "ML-X"; "DOT"; }
{ rank = same; "future"; "Adv. Software Technology"; }
"PEGASUS/PML" -> "ML-X";
"SCCS" -> "nmake";
"SCCS" -> "3D File System";
"SCCS" -> "RCS";
"make" -> "nmake";
"make" -> "build";
.
.
.
}

digraph G {

    splines=ortho

    A [ shape=box ]
    B [ shape=box ]
    C [ shape=box ]
    D [ shape=box ]

    A -> B
    A -> C

    B -> D
    C -> D

}


digraph G {

    graph [splines=ortho, nodesep=1]
    node [shape=record]

    A -> {B, C} -> D
}
"""
