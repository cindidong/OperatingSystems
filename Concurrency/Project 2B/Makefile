# NAME: Cindi Dong
# EMAIL: cindidong@gmail.com
# ID: 405126747


default: build

lab2_list: lab2_list.c SortedList.c SortedList.h
	gcc -g -Wall -Wextra -pthread -lrt lab2_list.c SortedList.c -o lab2_list

build: lab2_list

graphs: tests
	chmod u+x lab2_list.gp
	./lab2_list.gp 2> /dev/null

dist: graphs profile
	@tar -czvf lab2b-405126747.tar.gz lab2_list.c SortedList.c SortedList.h lab2b_list.csv profile.out lab2b_1.png lab2b_2.png lab2b_3.png lab2b_4.png lab2b_5.png lab2_list.gp lab2b_tests Makefile README

clean:
	rm -rf *.tar.gz *.o lab2_list

tests: build
	rm -f lab2b_list.csv
	chmod u+x lab2b_tests
	-./lab2b_tests 2>/dev/null

profile:
	rm -f lab2_list
	rm -f profile.out
	gcc -g -Wall -Wextra -pthread -lrt -L/u/be/ugrad/cindi/bin/lib -lprofiler lab2_list.c SortedList.c -o lab2_list
	rm -f ./raw.gperf
	LD_PRELOAD=/u/be/ugrad/cindi/bin/lib/libprofiler.so.0 CPUPROFILE=./raw.gperf ./lab2_list --iterations=1000 --threads=12 --sync=s
	/u/be/ugrad/cindi/bin/bin/pprof --text ./lab2_list ./raw.gperf > profile.out
	/u/be/ugrad/cindi/bin/bin/pprof --list=thread_func ./lab2_list ./raw.gperf >> profile.out
	rm -f ./raw.gperf
