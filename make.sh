#!/bin/sh -e

warnings='-Wall -Wfatal-errors'

missing_optional() { echo "optional '$1' not found"; }
missing_required() { echo "required '$1' not found, exiting!"; exit 1; }

# $1 is the program to test for, $2 is set if pkgconf's args should be ignored
check_optional() {
	pkg-config "$1" \
		&& {
			[ "$2" ] && eval "$(echo "$1" | tr '-' '_')='-DOML_$(echo "$1" | tr '-' '_' | tr "[:lower:]" "[:upper:]")'" \
			         || eval "$(echo "$1" | tr '-' '_')='$(pkg-config --libs --cflags $1) -DOML_$(echo "$1" | tr '-' '_' | tr "[:lower:]" "[:upper:]")'"
		} || missing_optional "$1"
}
check_required() {
	pkg-config "$1" \
		&& eval "$(echo "$1" | tr '-' '_')='$(pkg-config --libs --cflags $1) -DOML_$(echo "$1" | tr '-' '_' | tr "[:lower:]" "[:upper:]")'" \
		|| missing_required "$1"
}

check_optional valgrind 1
check_optional jack
check_optional x11
check_required json-c
check_required sndfile
[ -f "/usr/include/ladspa.h" ] && ladspa="-DOML_LADSPA" # jank
check_optional lilv-0

[ "$1" = "pg" ] && {
	set -x
	time gcc -o omelette -O0 \
		$valgrind $jack $sndfile $lilv_0 $ladspa $x11 $json_c \
		-lm $warnings -g -pg \
		src/main.c lib/libdrawille/src/liblibdrawille.a 2>&1
	# time gcc -o omuLADSPA -O0 \
	# 	$jack $ladspa \
	# 	-lm $warnings -g -pg \
	# 	src/ladspahost.c 2>&1
	time gcc -shared -o omelette-ladspa-plugins.so -O0 \
		-lm $warnings -g -pg -fPIC \
		src/ladspaplugins/ladspaplugins.c 2>&1
	killall -SIGUSR1 omuLADSPA ||:
	exit
}

[ "$1" ] && {
	set -x
	time ${CC:-gcc} -o omelette -O$1 \
		$valgrind $jack $sndfile $lilv_0 $ladspa $x11 $json_c \
		-lm $warnings -g \
		src/main.c lib/libdrawille/src/liblibdrawille.a 2>&1
	# time ${CC:-gcc} -o omuLADSPA -O$1 \
	# 	$jack $ladspa \
	# 	-lm $warnings -g \
	# 	src/ladspahost.c 2>&1
	time ${CC:-gcc} -shared -o omelette-ladspa-plugins.so -O$1 \
		-lm $warnings -g -fPIC \
		src/ladspaplugins/ladspaplugins.c 2>&1
	killall -SIGUSR1 omuLADSPA ||:
	exit
}

set -x
time ${CC:-tcc} -o omelette -O0 \
	$valgrind $jack $sndfile $lilv_0 $ladspa $x11 $json_c \
	-lm $warnings -g \
	src/main.c lib/libdrawille/src/liblibdrawille.a 2>&1
# time ${CC:-tcc} -o omuLADSPA -O0 \
# 	$jack $ladspa \
# 	-lm $warnings -g \
# 	src/ladspahost.c 2>&1
time ${CC:-tcc} -shared -o omelette-ladspa-plugins.so -O0 \
	-lm $warnings -g -fPIC \
	src/ladspaplugins/ladspaplugins.c 2>&1
killall -SIGUSR1 omuLADSPA ||:
