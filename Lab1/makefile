CC = clang
CFLAGS = -std=gnu17 -pedantic -Wall -Wextra -g
TARGET = main.out

.PHONY: clean run build

build: main.out child.out

main.out: main.o func.o
	${CC} -o $@ $^

child.out: child.o func.o
	${CC} -o $@ $^

main.o: main.c func.h
	${CC} ${CFLAGS} -c -o $@ $<

child.o: child.c func.h
	${CC} ${CFLAGS} -c -o $@ $<

misc.o: func.c func.h
	${CC} ${CFLAGS} -c -o $@ $<

clean:
	@rm -f ./*.o
	@rm -f ./*.out
	@rm -rf ./*.out.*