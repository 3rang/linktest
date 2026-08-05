#!/bin/sh
IP=$(hostname -I | awk '{print $1}')
printf '1\n' | ./build/linktest "$IP" -s &
sleep 1
printf '1\n' | ./build/linktest "$IP" -c
wait
