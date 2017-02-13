# Provenance To Use

Proof-of-concept for a Linux utility that develops the ability to provision a
complete runtime environment, dependencies, and program data for any Linux
application simply by studying a live sample of the application's runtime
behavior.

## Table of Contents

- [Example / Usage](#example--usage)
- [Getting Started](#getting-started)

## Example / Usage

1.

## Getting Started

### Ubuntu Linux

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
   # yum install gcc gcc-c++ git graphviz gv
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
disk space and 4GB of RAM.  Select the Cinnamon desktop environment option.

NOTE: Since Antergos is a rolling distribution based on Arch Linux, its "version
number" is best described as a given point in time; this setup was tested with
an up-to-date Antergos install on 09 FEB 2017.

2. Download the PTU application via Bitbucket:

   ```bash
   $ git clone https://username@bitbucket.org/tanum/provenance-to-use.git
   ```

   NOTE: to avoid future naming conflicts, do not rename the cloned directory to
   "PTU"

3. Change to the newly-cloned PTU directory:

   ```bash
   $ cd provenance-to-use
   ```

4. Compile and install PTU:

   ```bash
   $ make
   ```

5. Temporarily export the PTU directory location to the expected environment
variable.  For a permanent export, place this line in your user ~/.bash_profile
file.

   ```bash
   $ export PTU_HOME=/path/to/provenance-to-use
   ```

