CFLAGS := -g -std=c11 -fsanitize=address -fno-omit-frame-pointer -DENABLE_FFI=1
CC := clang
EXECUTABLE := catcat.exe

all: catcat.exe

catcat.exe: main.o lexer.o parser.o kernel.o
	$(CC) $(CFLAGS) $^ -o $@ -llibffi

main.o: main.c lexer.h kernel.h parser.h
	$(CC) $(CFLAGS) -c $< -o $@

lexer.o: lexer.c lexer.h
	$(CC) $(CFLAGS) -c $< -o $@

parser.o: parser.c parser.h lexer.h kernel.h
	$(CC) $(CFLAGS) -c $< -o $@

kernel.o: kernel.c kernel.h parser.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o *.exe

.PHONY: clean all