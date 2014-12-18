CC=g++
CFLAGS=-c -I..
SOURCES=args.h env.h logger.h mathutils.h path.h progress.h stringutils.h timeutils.h
OBJS=$(SOURCES:.h=.h.gch)

all: $(OBJS)

%.h.gch: %.h
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm $(OBJS)

.PHONY: all clean
