# NAME: Cindi Dong
# EMAIL: cindidong@gmail.com
# ID: 405126747


default: lab4b.c
	gcc -g -Wall -Wextra lab4b.c -o lab4b -lmraa -lm

check: default
	chmod u+x tests
	./tests

dist:
	tar -czvf lab4b-405126747.tar.gz lab4b.c tests Makefile README

clean:
	rm -f lab4b *.tar.gz

