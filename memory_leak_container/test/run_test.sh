#!/bin/bash
cd /data/src
rm ./quartz
make debug
echo '==========================='
ls
echo '==========================='
pwd
echo '==========================='
echo $PROGRAM
echo '==========================='
if [ -z "$PROGRAM" ]; then
    echo 'Program is not setted'
    prg=`ls -1 ./programs`
    echo $prg

    for value in $prg; do
        prog_path=`echo '/data/src/programs/'$value`
        echo '@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@'
        echo $prog_path
        echo '@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@'

        valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose /data/src/quartz $prog_path
    done
else
    valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose /data/src/quartz $PROGRAM
fi

