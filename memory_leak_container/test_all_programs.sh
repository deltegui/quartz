program_array=`ls -1 ../qcc/programs`
for value in $program_array; do
    echo '@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@'
    echo $value
    echo '@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@'
    sh ./test.sh $value
done
