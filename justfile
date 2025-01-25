#!/usr/bin/env just --justfile

default: build

# Compile and run `rom` on CChip8
run rom debug="true": (build debug)
    ./cchip8 {{ rom }}

# Compile CChip8
build debug="true":
    clang \
        -std=c23 \
        -march=native \
        -fuse-ld=mold \
        -Wextra \
        $(pkg-config sdl3 --cflags --libs) \
        -DDEBUG={{ debug }} \
        {{ if debug == "true" { "-g3 -O0" } else { "-O3" } }} \
        -o cchip8 \
        src/*.c
    chmod +x ./cchip8
