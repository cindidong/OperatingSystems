# NAME: Cindi Dong
# EMAIL: cindidong@gmail.com
# ID: 405126747


default: lab1b-server lab1b-client

lab1b-server: lab1b-server.c
	gcc -lz lab1b-server.c -o lab1b-server -g -Wall -Wextra 

lab1b-client: lab1b-client.c
	gcc -lz lab1b-client.c -o lab1b-client -g -Wall -Wextra

dist: default
	tar -czvf lab1b-405126747.tar.gz lab1b-client.c lab1b-server.c Makefile README

clean:
	rm -rf *.tar.gz lab1b-client lab1b-server
