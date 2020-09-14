# lab3b Makefile
# NAME: Arnold Pfahnl, Cindi Dong
# EMAIL: ajpfahnl@gmail.com, cindidong@gmail.com
# ID: 305176399, 405126747

default:
	@ln -sf lab3b.py lab3b
	@chmod u+x lab3b.py
	@chmod u+x lab3b
	@echo "Success"

tar_files = Makefile README lab3b.py

.PHONY: dist clean

clean:
	rm -f lab3b-305176399.tar.gz lab3b

dist:
	tar -z -c -f lab3b-305176399.tar.gz $(tar_files)
