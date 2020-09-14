# lab3a Makefile
# NAME: Arnold Pfahnl, Cindi Dong
# EMAIL: ajpfahnl@gmail.com, cindidong@gmail.com
# ID: 305176399, 405126747

all: lab3a

lab3a: lab3a.c
	gcc -o lab3a lab3a.c -Wall -Wextra

tar_files = Makefile README lab3a.c ext2_fs.h

.PHONY: dist clean

clean:
	rm -f lab3a-305176399.tar.gz lab3a

dist:
	tar -z -c -f lab3a-305176399.tar.gz $(tar_files)
