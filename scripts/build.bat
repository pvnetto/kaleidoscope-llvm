# rd /s /q "./build"
mkdir build
cd build
cmake ..
cmake --build . --config Debug
cd ../bin