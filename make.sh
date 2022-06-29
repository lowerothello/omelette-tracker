#!/bin/sh -e

run() {
	echo $@
	time $@
}

[ "$1" ] && {
	echo "optimised build"
	run ${CC:-gcc} $CFLAGS \
		$(pkg-config --libs --cflags jack) \
		$(pkg-config --libs --cflags sndfile) \
		$(pkg-config --libs --cflags lilv-0) \
		-lm -Wall -pg \
		src/main.c -o omelette
} || {
	run ${CC:-gcc} \
		$(pkg-config --libs --cflags jack) \
		$(pkg-config --libs --cflags sndfile) \
		$(pkg-config --libs --cflags lilv-0) \
		-lm -Wall -g -pg \
		src/main.c -o omelette
}
