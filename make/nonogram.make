NONOGRAM_C_FLAGS=-c -O2 -Wall -Wextra -Waggregate-return -Wcast-align -Wcast-qual -Wconversion -Wformat=2 -Winline -Wlong-long -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Wno-import -Wpointer-arith -Wredundant-decls -Wshadow -Wstrict-prototypes -Wwrite-strings

../bin/nonogram: ../obj/nonogram.o
	gcc -o ../bin/nonogram ../obj/nonogram.o

../obj/nonogram.o: ../src/nonogram.c nonogram.make
	gcc ${NONOGRAM_C_FLAGS} -o ../obj/nonogram.o ../src/nonogram.c

clean:
	rm -f ../bin/nonogram ../obj/nonogram.o
