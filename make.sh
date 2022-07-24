#!/bin/sh -e

run() {
	echo $@
	time $@
}

[ "$1" = "pg" ] && {
	echo "profiling build (-pg)"
	run gcc -o omelette -O0 \
		$(pkg-config --libs --cflags jack) \
		$(pkg-config --libs --cflags sndfile) \
		-lm -Wall -g -pg \
		src/main.c src/lib/libdrawille/src/liblibdrawille.a
	return
}

[ "$1" ] && {
	echo "optimised build (-O$1)"
	run ${CC:-tcc} -o omelette -O$1 \
		$(pkg-config --libs --cflags jack) \
		$(pkg-config --libs --cflags sndfile) \
		-lm -Wall -g \
		src/main.c src/lib/libdrawille/src/liblibdrawille.a
} || {
	run ${CC:-tcc} -o omelette -O0 \
		$(pkg-config --libs --cflags jack) \
		$(pkg-config --libs --cflags sndfile) \
		-lm -Wall -g \
		src/main.c src/lib/libdrawille/src/liblibdrawille.a
}
