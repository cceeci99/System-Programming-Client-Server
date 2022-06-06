#!/bin/bash

./remoteClient -p 5200 -i 195.134.65.88 -d Server/bar &&
./remoteClient -p 5200 -i 195.134.65.88 -d Server/foo &&
./remoteClient -p 5200 -i 195.134.65.88 -d Server/test/fold1 &&
./remoteClient -p 5200 -i 195.134.65.88 -d Server/test/fold2 &&
./remoteClient -p 5200 -i 195.134.65.88 -d Server/test/fold3 &&
./remoteClient -p 5200 -i 195.134.65.88 -d Server/test/fold4 &&
./remoteClient -p 5200 -i 195.134.65.88 -d Server/test/fold5