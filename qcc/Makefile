CC = clang
LIBS =

ifneq ($(OS),Windows_NT)
	UNAME := $(shell uname -s)
	ifeq ($(UNAME),Linux)
		LIBS += -lm
	endif
endif

release:
	$(CC) $(LIBS) -Wall ./*.c -o ./qcc

debug:
	$(CC) $(LIBS) -Wall -g -D DEBUG ./*.c -o ./qcc
