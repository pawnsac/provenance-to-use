#!/usr/bin/env python2

import leveldb, sys, string
printset = set(string.printable)
path = sys.argv[1] if len(sys.argv) > 1 else 'cde-package/provenance.cde-root.1.log_db'
totalrange = 20

db = leveldb.LevelDB(path, create_if_missing = False)
for (k,v) in db.RangeIter():
  if set(v).issubset(printset):
    print "[%s] '%s' -> [%s] '%s'" % (len(k), k, len(v), v)
  else:
    hexv = v.encode('hex')
    strv = ''.join([x if (ord(x) > 31 and ord(x)<128) else ('\\'+str(ord(x))) for x in v])
    if len(hexv) <= totalrange:
      print "[%s] '%s' -> [%s] '0x%s' %s" % (len(k), k, len(v), hexv, strv)
    else:
      hexv = hexv[:totalrange/2] + '...' + hexv[-totalrange/2:]
      print "[%s] '%s' -> [%s] '0x%s' %s" % (len(k), k, len(v), hexv, strv)
