#!/bin/bash
./lb 9001
for i in {9001..9003}; do
    ./lb $i &
done