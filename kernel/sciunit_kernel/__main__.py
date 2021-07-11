import os
import sys
import ast
import subprocess
import uuid
import socket
import signal
import stat
import pickle
import time
import shutil
from ipykernel.ipkernel import IPythonKernel

import sciunit_tree

SCIUNIT_HOME = os.path.expanduser('~/sciunit/')
SCIUNIT_PROJECT_FILE = os.path.join(SCIUNIT_HOME, '.activated')

PTU_PATH = shutil.which('ptu')
PTU_RESTORE_PATH = shutil.which('ptu-restore')

class SciunitKernel(IPythonKernel):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        implementation = super().implementation + ' sciunit'
        
        if (os.path.exists(SCIUNIT_PROJECT_FILE)):
            self.project = open(SCIUNIT_PROJECT_FILE).read().strip()
            self.project_name = os.path.basename(os.path.normpath(self.project))
        else:
            self.project_name = 'Project_' + str(uuid.uuid4())
            self.project = os.path.join(SCIUNIT_HOME, self.project_name)
            subprocess.run(['sciunit', 'create', self.project_name])

        self.socket_file = os.path.join(self.project, 'code.socket')
        if os.path.exists(self.socket_file):
            os.remove(self.socket_file)
        self.server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.server.bind(self.socket_file)
        os.chmod(self.socket_file, stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)
        self.server.listen(1)

        self.tree_path = os.path.join(self.project, 'tree.bin')
        if (os.path.exists(self.tree_path)):
            self.tree_root, self.subkernel_pid = sciunit_tree.tree_load(self.tree_path)
            subprocess.run(['sudo', 'kill', '-HUP', f'{self.subkernel_pid}'])
            self.subkernel = None
        else:
            self.tree_root = sciunit_tree.Tree()
            self.subkernel = subprocess.Popen(['sudo',
                                               PTU_PATH,
                                               sys.executable,
                                               'subkernel.py',
                                               self.project],
                                              stdin=subprocess.DEVNULL,
                                              stdout=subprocess.DEVNULL,
                                              stderr=subprocess.DEVNULL)
            conn, _ = self.server.accept()
            self.subkernel_pid = pickle.loads(conn.recv(1024))
            sciunit_tree.tree_dump(self.tree_root, self.subkernel_pid, self.tree_path)
        self.tree_node = self.tree_root
        
        self.tty = sys.__stdout__, sys.__stderr__

    def do_execute(self, code, silent, store_history=True, user_expressions=None, allow_stdin=False):
        added, child = self.tree_node.traverse(code)

        if added:
            if not self.subkernel:
                self.subkernel = subprocess.Popen(['sudo',
                                                   PTU_RESTORE_PATH,
                                                  f'cde-package/images/criu{sciunit_tree.hash_to_pyint(self.tree_node.hash)}'],
                                                  stdin=subprocess.DEVNULL,
                                                  stdout=subprocess.DEVNULL,
                                                  stderr=subprocess.DEVNULL)

            time.sleep(1)
            subprocess.run(['sudo', 'kill', '-USR1', f'{self.subkernel_pid}'])
            print('signal sent')

            conn, _ = self.server.accept()
            conn.sendall(pickle.dumps((code, child.hash)))
            result = pickle.loads(conn.recv(1024))
            if result.error_before_exec is None:
                self.tree_node = child
                time.sleep(1)
            else:
                self.tree_node.revert(child)

            sciunit_tree.tree_dump(self.tree_root, self.subkernel_pid, self.tree_path)

            print(result)
            if (hasattr(result, 'out')): print(result.out, end='')
        else:
            self.tree_node = child

        return super().do_execute("", silent, store_history, user_expressions, allow_stdin)

    def __del__(self):
        print('Destructor')
        subprocess.run(['sudo', 'kill', '-HUP', f'{self.subkernel_pid}'])
        self.server.close()
        os.unlink(self.socket_file)

if __name__ == '__main__':
    from ipykernel.kernelapp import IPKernelApp
    IPKernelApp.launch_instance(kernel_class=SciunitKernel)
