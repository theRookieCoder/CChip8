default: build

run rom debug="true": (build debug)
    ./cchip8 "ROMs/{{rom}}.ch8"

build debug="true":
    clang \
        -Wextra \
        -DDEBUG={{debug}} \
        $(pkg-config sdl3 --cflags --libs) \
        main.c \
        -o cchip8
    chmod +x ./cchip8
