CFLAGS = -Wall -Wextra -O2
TARGETS = read_test read_libc

all: $(TARGETS)

read_test: read_test.c
	$(CC) $(CFLAGS) -o read_test read_test.c

read_libc: read_libc.c
	$(CC) $(CFLAGS) -o read_libc read_libc.c

.PHONY: clean
clean:
	@rm -f $(TARGETS) large_file.bin