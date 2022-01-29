0. Request
cmake
make
g++ 9 
# if you got "undefined reference to `std::__cxx11::basic_stringstream<char,..." UPDATE to g++9
# This program need many CPU and MEMORY !!!

1. Download pdf2jpeg project
unpack project
cd pdfservice

2. install fastcommon
cd third

# this project use fastcommon Version 1.55  2022-01-12
# git clone https://github.com/fastdfs/libfastcommon.git
cd libfastcommon
./make.sh
# sudo ./make.sh install
cd ..

3. install fastdfs
# following the step on https://github.com/fastdfs/fastdfs/blob/master/INSTALL
# this project use fastdfs Version 6.07  2020-12-31
# git clone https://github.com/fastdfs/fastdfs.git
cd fastdfs
./make.sh
# sudo ./make.sh install
cd ..

3. install libturbojpeg

# following the step on https://github.com/libjpeg-turbo/libjpeg-turbo/blob/main/BUILDING.md
# this project use turbojpeg Version 2.1.2  2021-11-18
# git clone https://github.com/libjpeg-turbo/libjpeg-turbo.git
cd libjpeg-turbo
cmake .
make
# sudo make install
cd ..

4. build project
cd ..   # in pdfservice dirctory
cmake .
make



