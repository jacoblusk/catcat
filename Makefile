CFLAGS := -g -fsanitize=address -fno-omit-frame-pointer

all: catcat.exe

catcat.exe: main.o lexer.o parser.o kernel.o
	clang $(CFLAGS) $^ -o $@ -llibffi

main.o: main.c lexer.h kernel.h parser.h
	clang $(CFLAGS) -c $< -o $@

lexer.o: lexer.c lexer.h
	clang $(CFLAGS) -c $< -o $@

parser.o: parser.c parser.h lexer.h kernel.h
	clang $(CFLAGS) -c $< -o $@

kernel.o: kernel.c kernel.h parser.h
	clang $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o *.exe

.PHONY: clean all