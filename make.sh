#!/bin/sh -e

run() {
	echo $@
	time $@
}

echo "if this fails to link, run 'cd src/lib/libdrawille; cmake .; make; cd ../../..'"

[ "$1" ] && {
	echo "optimised build"
	run ${CC:-gcc} $CFLAGS \
		$(pkg-config --libs --cflags jack) \
		$(pkg-config --libs --cflags sndfile) \
		$(pkg-config --libs --cflags lilv-0) \
		-lm -Wall -pg \
		src/main.c -o omelette src/lib/libdrawille/src/liblibdrawille.a
} || {
	run ${CC:-gcc} \
		$(pkg-config --libs --cflags jack) \
		$(pkg-config --libs --cflags sndfile) \
		$(pkg-config --libs --cflags lilv-0) \
		-lm -Wall -g -pg \
		src/main.c -o omelette src/lib/libdrawille/src/liblibdrawille.a
}
