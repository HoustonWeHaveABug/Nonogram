NONOGRAM_C_FLAGS=-O2 -Wall -Wextra -Waggregate-return -Wcast-align -Wcast-qual -Wconversion -Wformat=2 -Winline -Wlong-long -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Wno-import -Wpointer-arith -Wredundant-decls -Wshadow -Wstrict-prototypes -Wwrite-strings

nonogram: nonogram.o
	gcc -o nonogram nonogram.o

nonogram.o: nonogram.c nonogram.make
	gcc -c ${NONOGRAM_C_FLAGS} -o nonogram.o nonogram.c

clean:
	rm -f nonogram nonogram.o
