#!/bin/sh -ex

warnings='-Wall'
include='-Iinclude'

# audio backends
pkg-config jack && jack="$(pkg-config --libs --cflags jack) -DOML_JACK"
# pkg-config libpulse && pulse="$(pkg-config --libs --cflags libpulse) -DOML_PULSE"

# audio file loading, TODO: should probably be required
pkg-config sndfile && sndfile="$(pkg-config --libs --cflags sndfile) -DOML_SNDFILE"

# X11 hack for key release events :3
pkg-config x11 && x11="$(pkg-config --libs --cflags x11) -DOML_X11"

# plugin backends
[ -f "/usr/include/ladspa.h" ] && ladspa="-DOML_LADSPA" # jank
pkg-config lilv-0 && lv2="$(pkg-config --libs --cflags lilv-0) -DOML_LV2"
# TODO: CLAP support

[ "$1" = "pg" ] && {
	time gcc -o omelette -O0 \
		$jack $pulse $sndfile $lv2 $ladspa $x11 \
		$include \
		-lm $warnings -g -pg \
		src/main.c lib/libdrawille/src/liblibdrawille.a 2>&1
	# time gcc -o omuLADSPA -O0 \
	# 	$jack $ladspa \
	# 	$include \
	# 	-lm $warnings -g -pg \
	# 	src/ladspahost.c 2>&1
	time gcc -shared -o omelette-ladspa-plugins.so -O0 \
		$include \
		-lm $warnings -g -pg -fPIC \
		src/ladspaplugins/ladspaplugins.c 2>&1
	killall -SIGUSR1 omuLADSPA ||:
	return
}

[ "$1" ] && {
	time ${CC:-gcc} -o omelette -O$1 \
		$jack $pulse $sndfile $lv2 $ladspa $x11 \
		$include \
		-lm $warnings -g \
		src/main.c lib/libdrawille/src/liblibdrawille.a 2>&1
	# time ${CC:-gcc} -o omuLADSPA -O$1 \
	# 	$jack $ladspa \
	# 	$include \
	# 	-lm $warnings -g \
	# 	src/ladspahost.c 2>&1
	time ${CC:-gcc} -shared -o omelette-ladspa-plugins.so -O$1 \
		$include \
		-lm $warnings -g -fPIC \
		src/ladspaplugins/ladspaplugins.c 2>&1
	killall -SIGUSR1 omuLADSPA ||:
	return
}

time ${CC:-tcc} -o omelette -O0 \
	$jack $pulse $sndfile $lv2 $ladspa $x11 \
	$include \
	-lm $warnings -g \
	src/main.c lib/libdrawille/src/liblibdrawille.a 2>&1
# time ${CC:-tcc} -o omuLADSPA -O0 \
# 	$jack $ladspa \
# 	$include \
# 	-lm $warnings -g \
# 	src/ladspahost.c 2>&1
time ${CC:-tcc} -shared -o omelette-ladspa-plugins.so -O0 \
	$include \
	-lm $warnings -g -fPIC \
	src/ladspaplugins/ladspaplugins.c 2>&1
killall -SIGUSR1 omuLADSPA ||:
return
