CONVERT_BITMAP_C_FLAGS=-O2 -Wall -Wextra -Waggregate-return -Wcast-align -Wcast-qual -Wconversion -Wformat=2 -Winline -Wlong-long -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -Wno-import -Wpointer-arith -Wredundant-decls -Wshadow -Wstrict-prototypes -Wwrite-strings

../bin/convert_bitmap: ../obj/convert_bitmap.o
	gcc -o ../bin/convert_bitmap ../obj/convert_bitmap.o

../obj/convert_bitmap.o: ../src/convert_bitmap.c convert_bitmap.make
	gcc -c ${CONVERT_BITMAP_C_FLAGS} -o ../obj/convert_bitmap.o ../src/convert_bitmap.c

clean:
	rm -f ../bin/convert_bitmap ../obj/convert_bitmap.o
