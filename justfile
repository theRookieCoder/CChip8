default: build-exec

run rom: build-exec
    ./cchip8 ROMs/{{rom}}.ch8

build-exec:
    clang \
        -Wextra \
        -DDEBUG \
        $(pkg-config sdl3 --cflags --libs) \
        main.c \
        -o cchip8
    chmod +x ./cchip8
