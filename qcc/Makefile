CC = clang
LIBS =

ifneq ($(OS),Windows_NT)
	UNAME := $(shell uname -s)
	ifeq ($(UNAME),Linux)
		LIBS += -lm
	endif
endif

release:
	$(CC) $(LIBS) ./*.c -o ./qcc

debug:
	$(CC) $(LIBS) -g -D DEBUG ./*.c -o ./qcc
