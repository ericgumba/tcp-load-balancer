#!/bin/bash
for i in {9001..9003}; do
    ./lb $i &
done