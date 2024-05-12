CC := gcc

SRCS := $(wildcard *.c)

OBJS := $(patsubst %.c, build/%.o, $(SRCS))

all: out

build/%.o: %.c 
	$(CC) -c $< -o $@

out : $(OBJS)
	$(CC) $^ -o $@

clean:
	rm -rf build/* 
	rm -rf /*.csv /*.out