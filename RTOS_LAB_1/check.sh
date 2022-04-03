#!/bin/bash

g++ main.cpp -lpthread -o otp.out

base64 /dev/urandom | head -c 2000 > input.txt # gen random file with base64
rm -f output.txt input2.txt input.txt
touch output.txt input2.txt input.txt

./otp.out  -i input.txt -o output.txt -x 4212 -a 84589 -c 45989 -m 217728 && ./otp.out  -i output.txt -o input2.txt -x 4212 -a 84589 -c 45989 -m 217728

if cmp -s input.txt input2.txt; then
    printf 'SUCCESS\n'
    # printf '\n'
    # cat input.txt
    # printf '\n'
    # cat output.txt
    # printf '\n' 
    # cat input2.txt
    # printf '\n'
else
    printf 'FAIL\n'
fi
