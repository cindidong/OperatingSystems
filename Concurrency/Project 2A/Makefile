# NAME: Cindi Dong
# EMAIL: cindidong@gmail.com
# ID: 405126747


default: build

lab2_add: lab2_add.c
	gcc -g -Wall -Wextra -pthread -lrt lab2_add.c -o lab2_add

lab2_list: lab2_list.c
	gcc -g -Wall -Wextra -pthread -lrt lab2_list.c SortedList.c -o lab2_list

build: lab2_add lab2_list

graphs: tests 
	@chmod u+x lab2_add.gp
	@chmod u+x lab2_list.gp
	@./lab2_add.gp 2> /dev/null
	@./lab2_list.gp 2> /dev/null

dist: graphs
	@tar -czvf lab2a-405126747.tar.gz lab2_add.c lab2_list.c SortedList.c SortedList.h lab2_add.csv lab2_list.csv lab2_add-1.png lab2_add-2.png lab2_add-3.png lab2_add-4.png lab2_add-5.png lab2_list-1.png lab2_list-2.png lab2_list-3.png lab2_list-4.png lab2_add.gp lab2_list.gp tests Makefile README

clean:
	rm -rf *.tar.gz *.o lab2_add lab2_list

tests: build
	rm -f lab2_add.csv 
	rm -f lab2_list.csv
	chmod u+x tests
	-./tests 2>/dev/null
