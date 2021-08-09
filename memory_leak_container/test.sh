!#/bin/bash

cp ../qcc/ ./src
docker-compose up
rm -rf ./src
