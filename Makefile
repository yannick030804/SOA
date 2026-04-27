all:
	cc -Wall -Wextra src/fsutils.c src/ext2.c src/fat16.c -o fsutils

clean:
	rm -f fsutils
