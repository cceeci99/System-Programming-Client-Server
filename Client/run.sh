#!/bin/bash

./client -p 5200 -i 195.134.65.88 -d bar > log1 &
./client -p 5200 -i 195.134.65.88 -d foo > log2 &
./client -p 5200 -i 195.134.65.88 -d test/fold1 > log3 &
./client -p 5200 -i 195.134.65.88 -d test/fold2 > log4 &
./client -p 5200 -i 195.134.65.88 -d test/fold3 > log5 &
./client -p 5200 -i 195.134.65.88 -d test/fold4 > log6 &
./client -p 5200 -i 195.134.65.88 -d test/fold5 > log7 &