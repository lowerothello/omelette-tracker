#!/bin/sh -e

run() {
	echo "$@"
	time "$@" 2>&1
}

[ "$1" = "pg" ] && {
	echo -e "\033[7m profiling build (gcc -pg) \033[27m"
	run gcc -o omelette -O0 \
		$(pkg-config --libs --cflags jack) \
		$(pkg-config --libs --cflags sndfile) \
		-lm -Wall -g -pg \
		src/main.c src/lib/libdrawille/src/liblibdrawille.a
	return
}

[ "$1" ] && {
	echo -e "\033[7m release build (${CC:-gcc} -O$1) \033[27m"
	run ${CC:-gcc} -o omelette -O$1 \
		$(pkg-config --libs --cflags jack) \
		$(pkg-config --libs --cflags sndfile) \
		-lm -Wall -g \
		src/main.c src/lib/libdrawille/src/liblibdrawille.a
	return
}

echo -e "\033[7m dev build (${CC:-tcc}) \033[27m"
run ${CC:-tcc} -o omelette -O0 \
	$(pkg-config --libs --cflags jack) \
	$(pkg-config --libs --cflags sndfile) \
	-lm -Wall -g \
	src/main.c src/lib/libdrawille/src/liblibdrawille.a
