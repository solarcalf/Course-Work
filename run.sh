cd ./src
g++ main.cpp RngStream.cpp -I./../include/ -std=c++17 -fopenmp -o out.exe
./out.exe
rm ./out.exe
cd ..