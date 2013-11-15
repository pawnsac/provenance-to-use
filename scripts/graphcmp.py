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
      vertices[key] = json.loads(value)
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
  
def checkVF2(G1, G2):
  ((vv1, eg1), (vv2, eg2)) = (G1, G2)
  
def main():
  if len(sys.argv) != 3:
    printUsage()
    exit(-1)
  
  G1 = getGraph(sys.argv[1])
  G2 = getGraph(sys.argv[2])
  if checkVF2(G1, G2):
    print "G1 is subgraph isomorphism of G2"
    return 1
  else:
    print "NOT subgraph isomorphism"
    return 0
    
main()