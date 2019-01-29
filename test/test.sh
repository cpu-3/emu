#!/bin/bash

tests="print sum-tail gcd sum fib even-odd \
    adder funcomp cls-rec cls-bug cls-bug2 cls-reg-bug \
    shuffle spill spill2 spill3 join-stack join-stack2 join-stack3 \
    join-reg join-reg2 non-tail-if non-tail-if2 \
    inprod inprod-rec"

function assertequal {
    [ "$1" != "$2" ] && fail "Test failed: $2 expected but got $1"
    echo -e "\033[0;32mok\033[0;39m"
}

function fail {
    echo -n -e "\033[0;31m[Error]\033[0;39m"
    echo "$1"
    exit 1
}

cd ..
make 2> /dev/null
cd "test"

for filename in $tests
do
    echo -n "$filename""..."
    min-caml $filename -inline 10 >/dev/null 2>/dev/null
    asm $filename".s" > /dev/null
    ret1="$(../emu a.out h 2> /dev/null)"
    ocamlc $filename".ml" 2> /dev/null
    ret2="$(./a.out)"
    assertequal "$ret1" "$ret2"
done
