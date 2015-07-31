#!/bin/bash
cd ../src
make
cd ../runtime
$TTROOT/src/tt -a -g zy -d ?Àgç <<DONE
report
class
stat
quit
quit
DONE
echo 'Done'
