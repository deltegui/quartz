#!/bin/bash

cp -a ../qcc/ ./test/src
export PROGRAM="$1"
docker-compose up
rm -rf ./test/src
docker-compose down
