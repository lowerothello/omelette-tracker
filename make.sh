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
		-lm -Wall -std=c99 \
		src/main.c -o omutrack
} || {
	run ${CC:-gcc} \
		$(pkg-config --libs --cflags jack) \
		$(pkg-config --libs --cflags sndfile) \
		$(pkg-config --libs --cflags lilv-0) \
		-lm -Wall -std=c99 \
		src/main.c -o omutrack
}
