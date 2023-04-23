CC = g++
CFLAGS = -O3 -Wall -Werror -std=c++11
OBJS = mmu.o pager.o

mmu: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

pager.o: pager.cc pager.hh
	$(CC) $(CFLAGS) $< -c -o $@

mmu.o: mmu.cc pager.hh
	$(CC) $(CFLAGS) $< -c -o $@

.PHONY: clean
clean:
	rm mmu $(OBJS)