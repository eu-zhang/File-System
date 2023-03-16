# Target library
lib := libfs.a

all: $(lib)

CC := gcc

CFLAGS := -Wall -Wextra -Werror -MMD

src := disk.c fs.c

obj := $(src:.c=.o)

$(lib): $(obj)
	ar rcs $@ $^

%.o: %.c disk.h fs.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(obj) $(lib)




