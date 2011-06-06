CFLAGS=-Wall -O3

all : tracepath

tracepath: main.o ipseeker.o
	$(CC) main.o ipseeker.o -o tracepath

clean:
	rm -f main.o ipseeker.o tracepath
