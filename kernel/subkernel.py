import os
import sys
import socket
import ctypes
import pickle
# from IPython.core.interactiveshell import InteractiveShell
import signal

import sciunit_tree

libc = ctypes.CDLL("libc.so.6")

project = sys.argv[1]
socket_file = os.path.join(project, 'code.socket')

# kernel = InteractiveShell(ipython_dir=project)
kernel = sciunit_tree.CodeKernel()

def handle_usr1(signum, stack):
    import psutil
    print(psutil.Process().open_files())
    client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    client.connect(socket_file)
    code, hash = pickle.loads(client.recv(1024))

    result = kernel.run_cell(code)
    print(result, len(pickle.dumps(result)))
    client.sendall(pickle.dumps(result))
    client.close()
    if result.error_before_exec is None:
        ret = libc.ptrace(-1, os.getpid(), 1, sciunit_tree.hash_to_cint(hash))
    print(f'Ptrace ret({sciunit_tree.hash_to_pyint(hash)})={ret}')

signal.signal(signal.SIGUSR1, handle_usr1)

client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
client.connect(socket_file)
client.sendall(pickle.dumps(os.getpid()))

if __name__ == '__main__':
    print(f'PID: {os.getpid()}')
    while True:
        signal.pause()