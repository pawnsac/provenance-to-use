#!/bin/sh
cd bash
./cde ./run.sh && cat provenance.log | grep -v \/proc\/
