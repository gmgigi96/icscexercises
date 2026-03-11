CFLAGS = -Wall -Wextra -O2
TARGET = read_test
SRCS = read_test.c

all: $(TARGET)

$(TARGET): $(SRCS)
	@$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

.PHONY: clean
clean:
	@rm -f $(TARGET) large_file.bin