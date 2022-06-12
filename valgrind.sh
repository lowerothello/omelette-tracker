#!/bin/sh -e

./make.sh # ensure the latest build

valgrind \
	--log-file=valgrind.log \
	--leak-check=full \
	./omelette
