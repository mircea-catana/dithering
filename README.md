# Dithering

Floyd-Steinberg dithering for PNG images.

## Build & Run


mkdir build

cd build

cmake ..

make


cp ../input.png

./dither

## Command Line Args

-i : path to input file

-o : path to output file

-b : number of bits/channel of output image [1,7]

