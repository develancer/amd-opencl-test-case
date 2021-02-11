CFLAGS=-std=gnu99 -Wall -Wextra -Wno-deprecated-declarations -O3
LDLIBS=-lOpenCL
.PHONY : clean default

default : weird

clean :
	@rm -vf weird
