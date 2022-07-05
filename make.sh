#!/bin/sh -e

run() {
	echo $@
	time $@
}

[ "$1" ] && {
	echo "optimised build (-O$1)"
	run ${CC:-tcc} -o omelette -O$1 \
		$(pkg-config --libs --cflags jack) \
		$(pkg-config --libs --cflags sndfile) \
		$(pkg-config --libs --cflags lilv-0) \
		-lm -Wall -g \
		src/main.c src/lib/libdrawille/src/liblibdrawille.a
} || {
	run ${CC:-tcc} -o omelette -O0 \
		$(pkg-config --libs --cflags jack) \
		$(pkg-config --libs --cflags sndfile) \
		$(pkg-config --libs --cflags lilv-0) \
		-lm -Wall -g \
		src/main.c src/lib/libdrawille/src/liblibdrawille.a
}
