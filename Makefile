# Thin frontend: keep build/install/run behavior in make.sh.
.DEFAULT_GOAL := build
.NOTPARALLEL:

ARGS ?=

.PHONY: all build release configure install run test clean purge help

all: build

build release configure install run test clean purge:
	@./make.sh $@ $(ARGS)

help:
	@./make.sh --help
	@printf '\nMake usage: make <action> [ARGS="options or game arguments"]\n'
	@printf '%s\n' 'Examples:' '  make run' '  make install' '  make run ARGS="-windowed"' '  make release ARGS="-j 4"'
