import pickle
import hashlib
import code
import io
import sys

HASH_SIZE = 4

class Tree:
    def __init__(self, hash=bytes(HASH_SIZE), code=''):
        self.hash = hash
        self.code = code
        self.children = {}
    def traverse(self, code):
        hash = hashlib.sha1(code.encode()).digest()[-HASH_SIZE:]
        hash = bytes(i ^ j for i, j in zip(hash, self.hash))
        added = False
        if hash not in self.children:
            self.children[hash] = Tree(hash, code)
            added = True
        return added, self.children[hash]
    def revert(self, child):
        del self.children[child.hash]

def tree_load(path):
    return pickle.load(open(path, "rb"))
def tree_dump(tree, pid, path):
    pickle.dump((tree, pid), open(path, "wb"))

def hash_to_cint(hash):
    return int.from_bytes(hash, 'little')
def hash_to_pyint(hash):
    pyint = hash_to_cint(hash)
    if pyint >= 2**(HASH_SIZE*8 - 1): pyint -= 2**(HASH_SIZE*8)
    return pyint

class CodeKernelResult:
    def __init__(self, error_before_exec=None, out=''):
        self.error_before_exec = error_before_exec
        self.out = out

class CodeKernel:
    def __init__(self):
        self.locals = {}
        self.interpreter = code.InteractiveInterpreter(locals=self.locals)
    def run_cell(self, code_in):
        old_stdout, old_stderr = sys.stdout, sys.stderr
        out = io.StringIO()
        sys.stdout = sys.stderr = out
        try:
            code_obj = code.compile_command(code_in, symbol='exec')
            code_in_except = None if code_obj else Exception()
        except Exception as e:
            code_obj = None
            code_in_except = e
        if code_obj: self.interpreter.runcode(code_obj)
        result = CodeKernelResult(code_in_except, out.getvalue())
        sys.stdout, sys.stderr = old_stdout, old_stderr
        return result