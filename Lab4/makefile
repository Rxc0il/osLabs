CC = cc
CFLAGS = -Wall -Wextra -lm -ldl
TARGET = main buddy.so biRange.so

build: ${TARGET}

main: main.c
	${CC} ${CFLAGS} -o $@ $<

buddy.so: AllocatorBuddy.c
	${CC} ${CFLAGS} -shared -fPIC -o $@ $<

biRange.so: AllocatorBiRange.c
	${CC} ${CFLAGS} -shared -fPIC -o $@ $<


clean:
	@rm -f ${TARGET}
	@rm -rf ./*.dSYM