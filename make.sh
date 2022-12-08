#!/bin/sh -ex

[ "$1" = "pg" ] && {
	time gcc -o omelette -O0 \
		$(pkg-config --libs --cflags jack) \
		$(pkg-config --libs --cflags sndfile) \
		$(pkg-config --libs --cflags lilv-0) \
		$(pkg-config --libs --cflags x11) \
		-lm -Wall -g -pg \
		src/main.c lib/libdrawille/src/liblibdrawille.a 2>&1
	time gcc -o omuLADSPA -O0 \
		$(pkg-config --libs --cflags jack) \
		-lm -Wall -g -pg \
		src/ladspahost.c 2>&1
	time gcc -shared -o omelette-ladspa-plugins.so -O0 \
		-lm -Wall -g -pg -fPIC \
		src/ladspaplugins/ladspaplugins.c 2>&1
	killall -SIGUSR1 omuLADSPA ||:
	return
}

[ "$1" ] && {
	time ${CC:-gcc} -o omelette -O$1 \
		$(pkg-config --libs --cflags jack) \
		$(pkg-config --libs --cflags sndfile) \
		$(pkg-config --libs --cflags lilv-0) \
		$(pkg-config --libs --cflags x11) \
		-lm -Wall -g \
		src/main.c lib/libdrawille/src/liblibdrawille.a 2>&1
	time ${CC:-gcc} -o omuLADSPA -O$1 \
		$(pkg-config --libs --cflags jack) \
		-lm -Wall -g \
		src/ladspahost.c 2>&1
	time ${CC:-gcc} -shared -o omelette-ladspa-plugins.so -O$1 \
		-lm -Wall -g -fPIC \
		src/ladspaplugins/ladspaplugins.c 2>&1
	killall -SIGUSR1 omuLADSPA ||:
	return
}

time ${CC:-tcc} -o omelette -O0 \
	$(pkg-config --libs --cflags jack) \
	$(pkg-config --libs --cflags sndfile) \
	$(pkg-config --libs --cflags lilv-0) \
	$(pkg-config --libs --cflags x11) \
	-lm -Wall -g \
	src/main.c lib/libdrawille/src/liblibdrawille.a 2>&1
time ${CC:-tcc} -o omuLADSPA -O0 \
	$(pkg-config --libs --cflags jack) \
	-lm -Wall -g \
	src/ladspahost.c 2>&1
time ${CC:-tcc} -shared -o omelette-ladspa-plugins.so -O0 \
	-lm -Wall -g -fPIC \
	src/ladspaplugins/ladspaplugins.c 2>&1
killall -SIGUSR1 omuLADSPA ||:
return
