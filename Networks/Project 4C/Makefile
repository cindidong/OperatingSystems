# NAME: Cindi Dong
# EMAIL: cindidong@gmail.com
# ID: 405126747


default:
	gcc -g -Wall -Wextra lab4c_tcp.c -o lab4c_tcp -lmraa -lm
	gcc -g -Wall -Wextra lab4c_tls.c -o lab4c_tls -lmraa -lm -lssl -lcrypto

dist:
	tar -czvf lab4c-405126747.tar.gz lab4c_tcp.c lab4c_tls.c Makefile README

clean:
	rm -f lab4c_tcp lab4c_tls *.tar.gz
