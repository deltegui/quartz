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

cd ./qcc
make sgc
make win64-sgc
cd ..
mv ./qcc/quartz ./dist/quartz-sgc
mv ./qcc/quartz.exe ./dist/quartz-sgc.exe

cd ./qcc
make debug-sgc
make win64-debug-sgc
cd ..
mv ./qcc/quartz ./dist/quartz-debug-sgc
mv ./qcc/quartz.exe ./dist/quartz-debug-sgc.exe

cp -a ./qcc/programs ./dist
zip -r quartz.zip ./dist
rm -r ./dist
