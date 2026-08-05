#!/bin/sh
./build/linktest 127.0.0.1 -s &
sleep 1
./build/linktest 127.0.0.1 -c
wait
