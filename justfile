default: build-exec

run: build-exec
    ./cchip8 "ROMs/IBM Logo.ch8"

build-exec:
    clang \
        -Wextra \
        -DDEBUG \
        $(pkg-config sdl3 --cflags --libs) \
        main.c \
        -o cchip8
    chmod +x ./cchip8
