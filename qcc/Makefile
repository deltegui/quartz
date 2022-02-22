CC = clang
LIBS =
SOURCES = ./*.c ./stdlib/*.c

ifneq ($(OS),Windows_NT)
	UNAME := $(shell uname -s)
	ifeq ($(UNAME),Linux)
		LIBS += -lm
	endif
endif

default:
	$(CC) $(LIBS) -Wall $(SOURCES) -o ./quartz

clean:
	rm ./quartz

release:
	$(CC) $(LIBS) -Wall -O3 $(SOURCES) -o ./quartz

win64:
	x86_64-w64-mingw32-gcc -lm -Wall -O1 $(SOURCES) -o ./quartz

debug:
	$(CC) $(LIBS) -Wall -Wextra -Wpedantic -Wno-unused-parameter -g -D DEBUG $(SOURCES) -o ./quartz

win64-debug:
	x86_64-w64-mingw32-gcc -lm -Wall -Wextra -Wpedantic -Wno-unused-parameter -g -D DEBUG $(SOURCES) -o ./quartz

sgc:
	$(CC) $(LIBS) -Wall -Wextra -Wpedantic -Wno-unused-parameter -g -D STRESS_GC $(SOURCES) -o ./quartz

win64-sgc:
	x86_64-w64-mingw32-gcc -lm -Wall -Wextra -Wpedantic -Wno-unused-parameter -g -D STRESS_GC $(SOURCES) -o ./quartz

debug-sgc:
	$(CC) $(LIBS) -Wall -Wextra -Wpedantic -Wno-unused-parameter -g -D DEBUG -D STRESS_GC $(SOURCES) -o ./quartz

win64-debug-sgc:
	x86_64-w64-mingw32-gcc -lm -Wall -Wextra -Wpedantic -Wno-unused-parameter -g -D DEBUG -D STRESS_GC $(SOURCES) -o ./quartz
