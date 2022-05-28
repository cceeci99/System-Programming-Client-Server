#!/bin/bash

./remoteClient -p 5200 -i 195.134.65.88 -d bar &&
./remoteClient -p 5200 -i 195.134.65.88 -d foo &&
./remoteClient -p 5200 -i 195.134.65.88 -d test/fold1 &&
./remoteClient -p 5200 -i 195.134.65.88 -d test/fold2 &&
./remoteClient -p 5200 -i 195.134.65.88 -d test/fold3 &&
./remoteClient -p 5200 -i 195.134.65.88 -d test/fold4 &&
./remoteClient -p 5200 -i 195.134.65.88 -d test/fold5