#!/bin/bash

./remoteClient -p 5200 -i 195.134.65.78 -d Server/bar > log1 &
./remoteClient -p 5200 -i 195.134.65.78 -d Server/others > log2 &
./remoteClient -p 5200 -i 195.134.65.78 -d Server/src > log3 &
./remoteClient -p 5200 -i 195.134.65.78 -d Server/test/fold1 > log3 &
./remoteClient -p 5200 -i 195.134.65.78 -d Server/test/fold2 > log4 &
./remoteClient -p 5200 -i 195.134.65.78 -d Server/test/fold3 > log5 &
./remoteClient -p 5200 -i 195.134.65.78 -d Server/test/fold4 > log6 &
./remoteClient -p 5200 -i 195.134.65.78 -d Server/test/fold5 > log7 &