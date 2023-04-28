all: mps mps_cv

mps: mps.c
	gcc -g -Wall -o mps -pthread mps.c -lrt -lm

mps_cv: mps_cv.c
	gcc -g -Wall -o mps_cv -pthread mps_cv.c -lrt -lm

clean:
	rm -fr *~ mps