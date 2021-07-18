rm ./quartz.zip
rm -r ./dist

cd ./qcc
make
make win64
cd ..
mkdir ./dist
mv ./qcc/quartz ./dist
mv ./qcc/quartz.exe ./dist

cd ./qcc
make debug
make win64-debug
cd ..
mv ./qcc/quartz ./dist/quartz-debug
mv ./qcc/quartz.exe ./dist/quartz-debug.exe

cp -a ./qcc/programs ./dist
zip -r quartz.zip ./dist
rm -r ./dist
