NONOGRAM_DEBUG_C_FLAGS=-c -g -Wall -Wextra -Waggregate-return -Wcast-align -Wcast-qual -Wconversion -Wformat=2 -Winline -Wlong-long -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Wno-import -Wpointer-arith -Wredundant-decls -Wshadow -Wstrict-prototypes -Wwrite-strings

../bin/nonogram_debug: ../obj/nonogram_debug.o
	gcc -g -o ../bin/nonogram_debug ../obj/nonogram_debug.o

../obj/nonogram_debug.o: ../src/nonogram.c nonogram_debug.make
	gcc ${NONOGRAM_DEBUG_C_FLAGS} -o ../obj/nonogram_debug.o ../src/nonogram.c

clean:
	rm -f ../bin/nonogram_debug ../obj/nonogram_debug.o
