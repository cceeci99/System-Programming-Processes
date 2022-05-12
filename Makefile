sniffer = ./src/sniffer

SNIFFER_OBJS = ./build/utils.o ./build/queue.o

worker = ./src/worker

WORKER_OBJS = ./build/utils.o ./build/urls.o ./build/work.o

all: ./src/sniffer.c $(SNIFFER_OBJS) ./src/worker.c $(WORKER_OBJS)
	gcc -o sniffer ./src/sniffer.c $(SNIFFER_OBJS)
	gcc -o worker ./src/worker.c $(WORKER_OBJS)

./build/utils.o: ./src/utils.c ./src/utils.h
	gcc -c -o ./build/utils.o ./src/utils.c

./build/queue.o: ./src/queue.c ./src/queue.h
	gcc -c -o ./build/queue.o ./src/queue.c

./build/urls.o: ./src/urls.c ./src/urls.h
	gcc -c -o ./build/urls.o ./src/urls.c

./build/work.o: ./src/work.c ./src/work.h
	gcc -c -o ./build/work.o ./src/work.c
		
run:		# default path is given to ./sniffer
	./sniffer

clean:
	rm ./build/* sniffer worker