#!/bin/bash
cd /data/src
rm ./quartz
make debug
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose "/data/src/quartz $PROGRAM"
