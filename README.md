# Provenance To Use

PTU is a Linux utility that captures a complete runtime environment,
dependencies, and program data - along with the associated provenance of each -
for any Linux application simply by studying a live sample of the application's
runtime behavior.  Subsequently, PTU can use the capture to reproduce and run
the whole application (or a specific subpart of it) on a bare Linux machine.

## Table of Contents

* [Terms And Definitions](#markdown-header-terms-and-definitions)
* [Installation](#markdown-header-installation)
    * [Fedora](#markdown-header-fedora)
        * [Fedora 25 Workstation](#markdown-header-fedora-25-workstation)
    * [Ubuntu](#markdown-header-ubuntu)
        * [Ubuntu Desktop 16-04](#markdown-header-ubuntu-desktop-16-04)
        * [Ubuntu Mate 16-10](#markdown-header-ubuntu-mate-16-10)
        * [Ubuntu Desktop 17-04](#markdown-header-ubuntu-desktop-17-04)
    * [CentOS](#markdown-header-centos)
        * [CentOS 7](#markdown-header-centos-7)
    * [Arch Linux](#markdown-header-arch-linux)
        * [Antergos 17-2](#markdown-header-antergos-17-2)
* [Usage](#markdown-header-usage)
    * [Capturing An Application](#markdown-header-capturing-an-application)
        * [Capture Commands](#markdown-header-capture-commands)
        * [Created Capture Files](#markdown-header-created-capture-files)
        * [Viewing The Capture Graphs](#markdown-header-viewing-the-capture-graphs)
    * [Running A Captured Application](#markdown-header-running-a-captured-application)
        * [Running The Whole Captured Application](#markdown-header-running-the-whole-captured-application)
        * [Running Only Part Of The Captured Application](#markdown-header-running-only-part-of-the-captured-application)
        * [Created Subprocess Run Files](#markdown-header-created-subprocess-run-files)
* [Architecture](#markdown-header-architecture)
    * [High Level Architecture](#markdown-header-high-level-architecture)
    * [Source Code Layout](#markdown-header-source-code-layout)
    * [Modules And Functions](#markdown-header-modules-and-functions)
* [Project Team](#markdown-header-project-team)
* [License](#markdown-header-license)

## Terms And Definitions

* __audit__: capture an application by running it with PTU
* __reference execution__: a single "run" of a captured application.  If the
  same application is captured multiple times, each run will produce a new
  reference execution.

## Installation

### Fedora

#### Fedora 25 Workstation

1. Install a Fedora 25 Workstation iso to a machine or VM with at least 20GB of
disk space and 4GB of RAM.

2. Install required PTU dependencies and create link to needed C++ library:

        $ dnf install gcc-c++ libstdc++-static graphviz gv gnuplot python2
        $ mkdir -p /usr/x86_64-redhat-linux/lib64/
        $ ln -s /usr/lib/gcc/x86_64-redhat-linux/6.3.1/libstdc++.so /usr/x86_64-redhat-linux/lib64/libstdc++.so

3. Download the PTU application via Bitbucket:

        $ git clone https://username@bitbucket.org/tanum/provenance-to-use.git

    NOTE: to avoid future naming conflicts, do not rename the cloned directory
    to "PTU"

4. Change to the newly-cloned PTU directory:

        $ cd provenance-to-use

5. Compile and install PTU:

        $ make

6. Temporarily export the PTU directory location to the expected environment
variable.  For a permanent export, place this line in your user
`~/.bash_profile` file.

        $ export PTU_HOME=/path/to/provenance-to-use

### Ubuntu

#### Ubuntu Desktop 16-04

1. Install an Ubuntu Desktop 16-04 iso to a machine or VM with at least 20GB of
disk space and 4GB of RAM.

2. Install required PTU dependencies:

        $ apt-get install git gv graphviz gnuplot libz-dev

3. Download the PTU application via Bitbucket:

        $ git clone https://username@bitbucket.org/tanum/provenance-to-use.git

    NOTE: to avoid future naming conflicts, do not rename the cloned directory
    to "PTU"

4. Change to the newly-cloned PTU directory:

        $ cd provenance-to-use

5. Compile and install PTU:

        $ make

6. Temporarily export the PTU directory location to the expected environment
variable.  For a permanent export, place this line in your user `~/.profile`
file.

        $ export PTU_HOME=/path/to/provenance-to-use

#### Ubuntu Mate 16-10

1. Install an Ubuntu Mate 16.10 iso to a machine or VM with at least 20GB of
disk space and 4GB of RAM.

2. Install required PTU dependencies:

        $ apt-get install gcc gcc-c++ make git gv graphviz gnuplot libz-dev

3. Download the PTU application via Bitbucket:

        $ git clone https://username@bitbucket.org/tanum/provenance-to-use.git

    NOTE: to avoid future naming conflicts, do not rename the cloned directory
    to "PTU"

4. Change to the newly-cloned PTU directory:

        $ cd provenance-to-use

5. Compile and install PTU:

        $ make

6. Temporarily export the PTU directory location to the expected environment
variable.  For a permanent export, place this line in your user `~/.profile`
file.

        $ export PTU_HOME=/path/to/provenance-to-use

#### Ubuntu Desktop 17-04

1. Install an Ubuntu Desktop 17-04 iso to a machine or VM with at least 20GB of
disk space and 4GB of RAM.

2. Install required PTU dependencies:

        $ apt-get install make git gv graphviz gnuplot zlib1g-dev

3. Download the PTU application via Bitbucket:

        $ git clone https://username@bitbucket.org/tanum/provenance-to-use.git

    NOTE: to avoid future naming conflicts, do not rename the cloned directory
    to "PTU"

4. Change to the newly-cloned PTU directory:

        $ cd provenance-to-use

5. Compile and install PTU:

        $ make

6. Temporarily export the PTU directory location to the expected environment
variable.  For a permanent export, place this line in your user `~/.profile`
file.

        $ export PTU_HOME=/path/to/provenance-to-use

### CentOS

#### CentOS 7

1. Install a CentOS 7.0 iso to a machine or VM with at least 20GB of disk space
and 4GB of RAM.  Select the Gnome graphical environment option.  Deselect the
Security Policy option (i.e. set "Apply Security Policy" to off, disabling
SELinux).

2. Install the EPEL respository (which contains the gv package), then install
required PTU dependencies and create link to needed C++ library:

        $ yum install epel-release
        $ yum install gcc gcc-c++ libstdc++-static git graphviz gv gnuplot
        $ mkdir -p /usr/x86_64-redhat-linux/lib64/
        $ ln -s /usr/lib/gcc/x86_64-redhat-linux/4.8.2/libstdc++.so /usr/x86_64-redhat-linux/lib64/libstdc++.so

3. Download the PTU application via Bitbucket:

        $ git clone https://username@bitbucket.org/tanum/provenance-to-use.git

    NOTE: to avoid future naming conflicts, do not rename the cloned directory
    to "PTU"

4. Change to the newly-cloned PTU directory:

        $ cd provenance-to-use

5. Compile and install PTU:

        $ make

6. Temporarily export the PTU directory location to the expected environment
variable.  For a permanent export, place this line in your user
`~/.bash_profile` file.

        $ export PTU_HOME=/path/to/provenance-to-use

### Arch Linux

#### Antergos 17-2

1. Install an Antegros 17-2 iso to a machine or VM with at least 20GB of
disk space and 4GB of RAM.  Select the KDE desktop environment option.

    NOTE: Since Antergos is a rolling distribution based on Arch Linux, its
    "version number" is best described as a given point in time; this setup was
    tested with an up-to-date Antergos install on 09 FEB 2017.

2. Install required PTU dependencies:

        $ pacman -S git graphviz gv

3. Download the PTU application via Bitbucket:

        $ git clone https://username@bitbucket.org/tanum/provenance-to-use.git

    NOTE: to avoid future naming conflicts, do not rename the cloned directory
    to "PTU"

4. Change to the newly-cloned PTU directory:

        $ cd provenance-to-use

5. Compile and install PTU:

        $ make

6. Temporarily export the PTU directory location to the expected environment
variable.  For a permanent export, place this line in your user
`~/.bash_profile` file.

        $ export PTU_HOME=/path/to/provenance-to-use

## Usage

### Capturing An Application

#### Capture Commands

To capture an application (i.e. either an executable binary, or script file),
use the ptu-audit script to run the application:

        $ /path/to/provenance-to-use/ptu-audit /path/to/binary-or-script [binary-or-script arguments]

For example:

        $ cd /home/user1
        $ /home/user1/provenance-to-use/ptu-audit /usr/bin/mutt -R

The application will launch and run normally.  As it runs, PTU will capture and
package the applications environment, dependencies, and data.  When you quit the
application, the complete packaging will be copied to a new "ptu" directory in
the current working directory.

#### Created Capture Files

NOTE: this section may be of more use to PTU developers, and may not be
pertinent to users only interested in capturing and running applications.

After PTU is finished capturing an application, it will create a "ptu" directory
in the current working directory. The ptu directory will contain the following
important files and directories:

* `cde-root`: a directory containing a snapshot/sandbox of all files and
executables used by the app, stored in a directory structure identical to the
structure on the host machine.
* `[binary-or-script-name.cde]`: a special "shortcut" script that will run the
captured application from within the sandbox.  It will be located within the
`cde-root` dir in the same location that the above capture command was run from
(e.g. for the capture command example above, `mutt.cde` will be located at
`/home/user1/ptu/cde-root/home/user1/mutt.cde`).
* `cde-exec`: an executable program utilized by the shortcut script.  This
program redirects calls made by the captured application into the `cde-root`
sandbox.
* `cde.uname`: a text file containing details about the host machine's
architecture and operating system.
* `cde.full-environment.cde-root`: a text file containing all the environment
variables that existed when the capture command was run.
* `cde.options`: a text file containing user-configurable options that modify
the behavior of `cde-exec`.
* `cde.log`: a text file containing the commands used to execute the original
application capture.
* `provenance.cde-root.1.log`: a log file of all the processes, files, system
memory, and other resources accessed by the captured app while it was running.
* `src`: a directory containing all PTU source code files; `cde-exec` will need
a working PTU program to run the captured application, and if such a program
does not exist, the source code provides the means to build one.
* `ptu-exec`: an executable script that will run only a specific part (i.e.
process) of a captured application.  If this file does not exist, the `src`
directory provides the means to build it.
* `runpid-py`: a python executable called by `ptu-exec` to run a specific part
of a captured application.
* `gv`: a directory containing a graph built by PTU that represents the
hierarchy of all the processes and files used by the captured application.
* `gv/main.html`: an html file containing a representation of the graph,
viewable in any browser.

#### Viewing The Capture Graphs

PTU produces many graphs that depict the captured application's process
hierarchy, memory usage, etc.  You can view them using firefox (or your browser
of choice) with the following command:

        $ firefox ptu/gv/main.html

### Running A Captured Application

#### Running The Whole Captured Application

1. Change to the `ptu/cde-root` sandbox directory.

2. Change to the directory within `cde-root` that contains the shortcut script
file `[binary-or-script-name.cde]` (See
[Created Capture Files](#markdown-header-created-capture-files) above for
explanation).

3. Run the shortcut script file by invoking the same command (with the same
options) that was run to capture the application:

        $ /path/to/cde-root/path/to/binary-or-script.cde [binary-or-script arguments]

    For the above example, the following command would run the captured application:

        $ /home/user1/ptu/cde-root/home/user1/mutt.cde -R

#### Running Only Part Of The Captured Application

1. Obtain the process ID (PID) for the subprocess (i.e. "subpart") of the
captured application you wish to run.  This PID can be found by viewing the
graph of the captured application (`ptu/gv/main.html`) or by viewing the capture
log (`provenance.cde-root.1.log`).

2. Change to the `ptu` directory.

3. Run the subprocess of the captured application:

        $ ./ptu-exec -p [PID]

#### Created Subprocess Run Files

NOTE: this section may be of more use to PTU developers, and may not be
pertinent to users only interested in capturing and running applications.

When PTU runs a subprocess of an application, it will create the following
files and directories within the `ptu` directory:

* `.temp/.[PID]`: a directory similar in content and structure to `cde-root`,
but containing only those files and executables needed by the subprocess
(instead of the entire app).
* `.[PID].cde`: an executable script used by `ptu-exec` to run the subprocess.

## Architecture

### High Level Architecture

TODO: architecture diagram

### Source Code Layout

NOTE: files and directories annotated with a `*` are fixed dependencies modified
by this project.  Files and directories NOT annotated with a `*` have been
created by this project.

```
provenance-to-use/
├── strace-4-6*/        # Capture app, run app, track provenance
│   ├── /db.c           # Store or retrieve prov data from a leveldb database
│   ├── /defs.h*        # Data structures and macros for tracing an app
│   ├── /desc.c*        # System calls for file close, fd dup, and other misc
│   ├── /cde.c          # Audit app or run captured app
│   ├── /cdenet.c       # Audit, run, or record network prov (experimental)
│   ├── /file.c*        # System calls to trace file access
│   ├── /okapi.c        # Copy files/dirs/simlinks with structural fidelity
│   ├── /printenv.c     # Retrieve a process's environment variables
│   ├── /process.c*     # System calls to trace process actions
│   ├── /provenance.c   # Record app prov info to text log and to database
│   ├── /strace.c*      # Main entry point: runs and traces app for audit/capture
│   ├── /syslimits.c    # Obtain OS maxes for num open files, command-line length, etc.
├── readelf-mini*/      # Read contents of files by file type
├── snappy-1.1.1*/      # Compress/store captured files
├── leveldb-1.14.0*/    # Store capture graph
├── scripts/            # Perform various utility/intermediate functions
│   ├── /prov2dot.py    # Create the capture graph
│   ├── /runpid.py      # Run subprocess of captured app
├── ptu-audit           # Capture app
├── ptu-exec            # Run subprocess of captured app (call runpid.py)
```

### Modules And Functions

NOTE: files and directories annotated with a `*` are fixed dependencies modified
by this project.  Files and directories NOT annotated with a `*` have been
created by this project.

```
├── provenance.c                  # Record app prov info to text log and to database
│   ├── init_prov()               # Initialize prov text log and prov database
│   ├── print_begin_execve_prov() # Log process execve (starting info) sys call
│   ├── print_end_execve_prov()   # Log process execve (ending info) sys call
│   ├── print_spawn_prov()        # Log process creation of new process
│   ├── print_open_prov()         # Log file open/openat sys call
│   ├── print_read_prov()         # Log file read sys call
│   ├── print_write_prov()        # Log file write sys call
│   ├── print_link_prov()         # Log file hardlink/symlink creation sys call
│   ├── print_rename_prov()       # Log file rename/move sys call
│   ├── print_close_prov()        # Log file close sys call
├── desc.c*                       # System calls for file close, fd dup, and other misc
│   ├── sys_close()*              # Sys call: close file
├── defs.h*                       # Data structures and macros for tracing an app
│   ├── entering()*               # Return T if app just made syscall, F if kernel just processed syscall
├── file.c*                       # System calls to trace file access
│   ├── sys_open()*               # Sys call: open file
│   ├── sys_openat()*             # Sys call: open file relative to specified dir
```

## Project Team

TODO

## License

Distributed under the GPLv3 license.

