CFLAGS = -Wall -Wextra -O2

server_uring: LDLIBS += -luring
server_uring_zerocopy: LDLIBS += -luring
SRCS = $(wildcard *.c)
TARGETS = $(SRCS:.c=)

all: $(TARGETS)

%: %.c
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

clean:
	rm -f $(TARGETS) 1GB_dataset.bin

.PHONY: all clean