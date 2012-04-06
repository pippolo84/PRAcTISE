LDFLAGS=-lm

all: stats

stats: stats.o
	$(CC) -o $@ $< $(LDFLAGS)

%.o: %.c
	$(CC) -o $@ -c $<

.PHONY: clean
clean: 
	rm stats *.o
