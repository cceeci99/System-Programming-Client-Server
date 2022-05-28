#!/bin/bash

./remoteClient -p 5200 -i 195.134.65.88 -d bar > log1 &
./remoteClient -p 5200 -i 195.134.65.88 -d foo > log2 &
./remoteClient -p 5200 -i 195.134.65.88 -d test/fold1 > log3 &
# ./remoteClient -p 5200 -i 195.134.65.88 -d test/fold2 > log4 &
# ./remoteClient -p 5200 -i 195.134.65.88 -d test/fold3 > log5 &
# ./remoteClient -p 5200 -i 195.134.65.88 -d test/fold4 > log6 &
# ./remoteClient -p 5200 -i 195.134.65.88 -d test/fold5 > log7 &