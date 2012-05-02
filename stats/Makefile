CFLAGS= -Wall -Wextra -ansi -O0 -ggdb3

all: extract_event_occurences stats_coloumn

extract_event_occurences: extract_event_occurences.o

stats_coloumn: stats_coloumn.o

.PHONY: clean
clean:
	rm -f extract_event_occurences stats_coloumn *.o
