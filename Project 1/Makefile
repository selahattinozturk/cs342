all: proctopk threadtopk

threadtopk: threadtopk.c
	gcc -Wall -o threadtopk threadtopk.c -lpthread

proctopk: proctopk.c
	gcc -Wall -o proctopk proctopk.c -lrt

clean: 
	rm -fr *~ proctopk threadtopk
korpe@ubuntu:~/x$ ls