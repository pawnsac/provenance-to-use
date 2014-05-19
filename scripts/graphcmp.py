#!/usr/bin/python

import os, re, sys, json

def printUsage():
  print "Usage:"
  print "  " + sys.argv[0] + " <graph1> <graph2>"
  
def openFile(filename):
  try:
    f = open(filename, "rb")
  except IOError:
    print "Could not open file '" + filename + '"!'
    exit(-1)
  return f

def getVertices(filename):
  g = openFile(filename)
  vertices = {}
  while 1:
    line = g.readline()
    if line == '':
      break;
    line = line.rstrip('\n')
    if line.find(" : ") > 0:
      [key, value] = line.split(" : ", 2)
      vertices[key] = value # keep it as string for fast comparison
  g.close()
  return vertices

def getEdges(filename):
  g = openFile(filename)
  in_edge = {}
  out_edge = {}
  while 1:
    line = g.readline()
    if line == '':
      break;
    line = line.rstrip('\n')
    if line.find(" -> ") > 0:
      [v1, v2] = line.split(" -> ", 2)
      if v1 in out_edge:
        out_edge[v1].append(v2)
      else:
        out_edge[v1] = [v2]
      if v2 in in_edge:
        in_edge[v2].append(v1)
      else:
        in_edge[v2] = [v1]
  g.close()
  return (out_edge, in_edge)

def getGraph(filename):
  return (getVertices(filename), getEdges(filename))
  
def edge_match(v1, v2, G1, G2):
  return 1
  
# use VF2 implementation, roughly described in
# http://stackoverflow.com/questions/6743894/any-working-example-of-vf2-algorithm/6744603#6744603
def isSubgraphISM(G1, G2):
  ((vv1, (out_eg1, in_eg1)), (vv2, (out_eg2, in_eg2))) = (G1, G2)

  # initialization
  vv1key = vv1.keys()
  vv1len = len(vv1key)
  vv2key = vv2.keys()
  vv2len = len(vv2key)
  vv1match = {}     # dict[v1] -> v2
  vv2match = {}     # dict[v2] -> v1
  id = []     # vf2match[i] = index of dict[vv1[i]] in vv2key
  for v in vv1key:
    id.append(-1)
    
  # vf2 algorithm
  i = 0
  while i < vv1len:
    print 'i'+str(i)
    v1 = vv1key[i] # current v1 to get matched
    id[i] += 1     # get next value of v2 to match
    while id[i] < vv2len:
      print 'id[i]:'+str(id[i])
      v2 = vv2key[id[i]]
      if v2 not in vv2match.keys() \
      and vv1[v1] == vv2[v2]:    # v2 not match yet and has path and dir match each other
        if edge_match(v1, v2, G1, G2):
          vv1match[v1] = v2   # keep the match
          vv2match[v2] = v1
          if i == vv1len - 1:
            print vv1match
          else:
            i += 1
          break # out of id[i] loop
      else:
        id[i] += 1
    if id[i] == vv2len: #no match for id[i], try another id[i-1] 
      id[i] = -1
      i -= 1
      if i<0:
        break # out of i loop
    else: # find a match for id[i], continue with id[i+1]
      i += 1

def perm(l1, vv2):
  l2 = len(vv2)
  id = []
  s1 = {}
  
  id = [-1 | i in 0:l1]
  
  i = 0
  while i < l1:
    if id[i] >= 0:
      s.remove(id[i])
    id[i] += 1
    if (id[i] == l2):
      id[i] = -1
    else:
      s[id[i]] = 1
      i += 1
      if i == l1:
        print id
    
    
  
def main():
  if len(sys.argv) != 3:
    printUsage()
    exit(-1)
  
  G1 = getGraph(sys.argv[1])
  G2 = getGraph(sys.argv[2])
  perm(3, [4, 5, 6, 7])
  return
  if isSubgraphISM(G1, G2):
    print "G1 is subgraph isomorphism of G2"
    return 1
  else:
    print "NOT subgraph isomorphism"
    return 0
    
main()

pseudo_code = """Algorithm:

First step : match empty V with empty graph V'.
Second step : try to math one node in V with one node in V'
...
Nth step : try to math one node in V with one new node in V', 
if you cannot go back one step in the tree and try a new match in the previous step.. 
and if you cant go back again etc...
...
END : when you find a leaf, i.e when you went through all the nodes in V' or when you went through the all tree without finding a leaf."""