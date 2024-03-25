#!/bin/bash

test_compile() {
    sleep 0.25 # Sleeping to avoid permission issues with file overwriting
    gcc scrypt-jane-speed.c -O3 -DSCRYPT_$1 -DSCRYPT_$2 $3 -o scrypt_speed 2>/dev/null
    local RC=$?
    if [ $RC -ne 0 ]; then
        echo "$1/$2: failed to compile"
        return 1
    fi
    return 0
}

test_hash() {
    test_compile $1 SALSA $2
    test_compile $1 CHACHA $2
    test_compile $1 SALSA64 $2
}

test_hashes() {
    test_hash SHA256 $1
    test_hash SHA512 $1
    test_hash BLAKE256 $1
    test_hash BLAKE512 $1
    test_hash SKEIN512 $1
    test_hash KECCAK256 $1
    test_hash KECCAK512 $1
}

if [ -z $1 ]; then
    test_hashes
elif [ $1 -eq 32 ]; then
    test_hashes -m32
elif [ $1 -eq 64 ]; then
    test_hashes -m64
fi

rm -f scrypt_speed
