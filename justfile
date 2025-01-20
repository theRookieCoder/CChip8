default: build

run rom debug="true" +others="": (build debug others)
    ./cchip8 "ROMs/{{rom}}.ch8"

build debug="true" +others="":
    clang {{others}} \
        -std=c23 \
        -march=native \
        -fuse-ld=mold \
        -Wextra \
        $(pkg-config sdl3 --cflags --libs) \
        {{ if debug == "true" { "-g3 -O0 -DDEBUG=true" } else { "-O3 -UDEBUG" } }} \
        src/*.c \
        -o cchip8
    chmod +x ./cchip8
