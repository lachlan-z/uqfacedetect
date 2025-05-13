CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=gnu99 -g
INCLUDES = -I/local/courses/csse2310/include
LIBS = -L/local/courses/csse2310/lib -ltinyexpr -lm

all: uqfaceclient

uqexpr: uqfaceclient.c
	$(CC) $(CFLAGS) uqfaceclient.c -o uqfaceclient
