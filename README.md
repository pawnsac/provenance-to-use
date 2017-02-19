# Provenance To Use

Proof-of-concept for a Linux utility that captures a complete runtime
environment, dependencies, and program data for any Linux application simply by
studying a live sample of the application's runtime behavior.  Subsequently, PTU
can use the capture to reproduce and run the application on a bare Linux
machine.

## Table of Contents

- [Installation](#installation)
- [Usage](#usage)

## Installation

### Fedora

#### Fedora 25 Workstation

1. Install a Fedora 25 Workstation iso to a machine or VM with at least 20GB of
disk space and 4GB of RAM.

2. Install required PTU dependencies and create link to needed C++ library:

```bash
# dnf install gcc-c++ libstdc++-static graphviz gv python2
# mkdir -p /usr/x86_64-redhat-linux/lib64/
# ln -s /usr/lib/gcc/x86_64-redhat-linux/6.3.1/libstdc++.so /usr/x86_64-redhat-linux/lib64/libstdc++.so
```

3. Download the PTU application via Bitbucket:

```bash
$ git clone https://username@bitbucket.org/tanum/provenance-to-use.git
```

NOTE: to avoid future naming conflicts, do not rename the cloned directory to
"PTU"

4. Change to the newly-cloned PTU directory:

```bash
$ cd provenance-to-use
```

5. Compile and install PTU:

```bash
$ make
```

6. Temporarily export the PTU directory location to the expected environment
variable.  For a permanent export, place this line in your user ~/.bash_profile
file.

```bash
$ export PTU_HOME=/path/to/provenance-to-use
```

### Ubuntu

#### Ubuntu Mate 16.10

1. Install an Ubuntu Mate 16.10 iso to a machine or VM with at least 20GB of
disk space and 4GB of RAM.


2. Install required PTU dependencies:

```bash
# apt-get install gcc gcc-c++ make git gv graphviz libz-dev
```

3. Download the PTU application via Bitbucket:

```bash
$ git clone https://username@bitbucket.org/tanum/provenance-to-use.git
```

NOTE: to avoid future naming conflicts, do not rename the cloned directory to
"PTU"

4. Change to the newly-cloned PTU directory:

```bash
$ cd provenance-to-use
```

5. Compile and install PTU:

```bash
$ make
```

6. Temporarily export the PTU directory location to the expected environment
variable.  For a permanent export, place this line in your user ~/.profile file.

```bash
$ export PTU_HOME=/path/to/provenance-to-use
```

### CentOS

#### CentOS 7

1. Install a CentOS 7.0 iso to a machine or VM with at least 20GB of disk space
and 4GB of RAM.  Select the Gnome graphical environment option.  Deselect the
Security Policy option (i.e. set "Apply Security Policy" to off, disabling
SELinux).

2. Install the EPEL respository (which contains the gv package), then install
required PTU dependencies and create link to needed C++ library:

```bash
# yum install epel-release
# yum install gcc gcc-c++ libstdc++-static git graphviz gv
# mkdir -p /usr/x86_64-redhat-linux/lib64/
# ln -s /usr/lib/gcc/x86_64-redhat-linux/4.8.2/libstdc++.so /usr/x86_64-redhat-linux/lib64/libstdc++.so
```

3. Download the PTU application via Bitbucket:

```bash
$ git clone https://username@bitbucket.org/tanum/provenance-to-use.git
```

NOTE: to avoid future naming conflicts, do not rename the cloned directory to
"PTU"

4. Change to the newly-cloned PTU directory:

```bash
$ cd provenance-to-use
```

5. Compile and install PTU:

```bash
$ make
```

6. Temporarily export the PTU directory location to the expected environment
variable.  For a permanent export, place this line in your user ~/.bash_profile
file.

```bash
$ export PTU_HOME=/path/to/provenance-to-use
```

### Arch Linux

#### Antergos 17-2

1. Install an Antegros 17-2 iso to a machine or VM with at least 20GB of
disk space and 4GB of RAM.  Select the KDE desktop environment option.

NOTE: Since Antergos is a rolling distribution based on Arch Linux, its "version
number" is best described as a given point in time; this setup was tested with
an up-to-date Antergos install on 09 FEB 2017.

2. Install required PTU dependencies:

```bash
# pacman -S git graphviz gv
```

3. Download the PTU application via Bitbucket:

```bash
$ git clone https://username@bitbucket.org/tanum/provenance-to-use.git
```

NOTE: to avoid future naming conflicts, do not rename the cloned directory to
"PTU"

4. Change to the newly-cloned PTU directory:

```bash
$ cd provenance-to-use
```

5. Compile and install PTU:

```bash
$ make
```

6. Temporarily export the PTU directory location to the expected environment
variable.  For a permanent export, place this line in your user ~/.bash_profile
file.

```bash
$ export PTU_HOME=/path/to/provenance-to-use
```

## Usage

### Capturing An Application

To capture an application (i.e. either an executable binary, or script file),
you use the ptu-audit script to run the application:

```bash
$ /path/to/provenance-to-use/ptu-audit /path/to/binary-or-script [binary-or-script arguments]
```

For example:

```bash
$ cd /home/user1
$ /home/user1/provenance-to-use/ptu-audit /usr/bin/mutt -R
```

The application will launch and run normally.  As it runs, PTU will capture and
package the applications environment, dependencies, and data.  When you quit the
application, the complete packaging will be copied to a new "ptu" directory in
the current working directory.  The "ptu" directory will contain the following
important files and directories:

* `cde-root` dir: a snapshot/sandbox of all files and executables used by the
app, stored in a directory structure identical to the structure on the host
machine.
* `[binary-or-script-name.cde]` file: a special executable file that will run
the captured application from within the sandbox.  It will be located within the
`cde-root` dir in the same location that the above capture command was run from
(e.g. for the capture command example above, `mutt.cde` will be located at
`/home/user1/ptu/cde-root/home/user1/mutt.cde`).
* `cde.options` file: a file containing all the the environment variables that
existed when the capture command was run.
* `provenance.cde-root.1.log` file: a log of all the files and system memory
locations accessed by the captured app while it was running.
* `gv` dir: a graph built by PTU that represents the hierarchy of all the
processes and files used by the captured application.
* `gv/main.html` file: an html representation of the graph, viewable in any
browser.


