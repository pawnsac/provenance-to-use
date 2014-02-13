#!/bin/sh
cp ../../ptu-ex cde-package/cde-exec
rm -rf cde-package/provenance.cde-root.1.log_db.*

### rerun
cd cde-package/cde-root/home/quanpt/assi/cde/mytest/mpich2
../../../../../../../cde-exec $@ -N provenance.cde-root.1.log_db mpirun -n 2 -f host_file ./mpi_hello_world
