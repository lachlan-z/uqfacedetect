CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=gnu99 -g
LIBS = -L/usr/lib64 -lopencv_core -lopencv_imgcodecs -lopencv_objdetect -lopencv_imgproc

all: uqfaceclient uqfacedetect

uqfaceclient: uqfaceclient.c
	$(CC) $(CFLAGS) uqfaceclient.c -o uqfaceclient

uqfacedetect: uqfacedetect.c
	$(CC) $(CFLAGS) uqfacedetect.c -o uqfacedetect $(LIBS)
