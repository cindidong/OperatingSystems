# NAME: Cindi Dong
# EMAIL: cindidong@gmail.com
# ID: 405126747

CC = gcc
CFLAGS = -g -Wall -Wextra

default: lab1a

lab1a: lab1a.c
	$(CC) $(CFLAGS) lab1a.c -o lab1a

dist: default
	tar -czvf lab1a-405126747.tar.gz lab1a.c Makefile README

clean:
	rm -rf *.tar.gz lab1a
