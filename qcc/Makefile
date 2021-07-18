CC = clang
LIBS =

ifneq ($(OS),Windows_NT)
	UNAME := $(shell uname -s)
	ifeq ($(UNAME),Linux)
		LIBS += -lm
	endif
endif

release:
	$(CC) $(LIBS) -Wall -O1 ./*.c -o ./quartz

debug:
	$(CC) $(LIBS) -Wall -Wextra -Wpedantic -Wno-unused-parameter -g -D DEBUG ./*.c -o ./quartz
