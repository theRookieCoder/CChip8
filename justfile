default: build

run rom debug="true" +others="": (build debug others)
    ./cchip8 "ROMs/{{rom}}.ch8"

build debug="true" +others="":
    clang {{others}} \
        -std=c23 \
        -march=native \
        -fuse-ld=mold \
        -O3 \
        -Wextra \
        -DDEBUG={{debug}} \
        $(pkg-config sdl3 --cflags --libs) \
        src/*.c \
        -o cchip8
    chmod +x ./cchip8
