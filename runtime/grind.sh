#!/bin/bash
cd ../src
make
cd ../runtime
$TTROOT/src/tt -a -g zy -d ?Àgç -f grind.tt
cd $TTROOT/ttkb
echo 'Sorting...'
./sort.sh
echo 'Done'
