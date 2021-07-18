rm ./quartz.zip
rm -r ./dist
cd ./qcc
make
make win64
cd ..
mkdir ./dist
mv ./qcc/quartz ./dist
mv ./qcc/quartz.exe ./dist
cp -a ./qcc/programs ./dist
zip -r quartz.zip ./dist
rm -r ./dist
