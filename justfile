default: build-exec

run: build-exec
    ./cchip8 "ROMs/IBM Logo.ch8"

build-exec:
    clang -Wextra main.c -DDEBUG -o cchip8;
    chmod +x ./cchip8
