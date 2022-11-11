NONOGRAM_PG_C_FLAGS=-c -pg -Wall -Wextra -Waggregate-return -Wcast-align -Wcast-qual -Wconversion -Wformat=2 -Winline -Wlong-long -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Wno-import -Wpointer-arith -Wredundant-decls -Wshadow -Wstrict-prototypes -Wwrite-strings

../bin/nonogram_pg: ../obj/nonogram_pg.o
	gcc -pg -o ../bin/nonogram_pg ../obj/nonogram_pg.o

../obj/nonogram_pg.o: ../src/nonogram.c nonogram_pg.make
	gcc ${NONOGRAM_PG_C_FLAGS} -o ../obj/nonogram_pg.o ../src/nonogram.c

clean:
	rm -f ../bin/nonogram_pg ../obj/nonogram_pg.o
