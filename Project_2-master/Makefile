CFLAGS = -Wall -g -std=gnu99 -pthread
CC = gcc $(CFLAGS)
 
bl-client : server.o bl-client.o util.o simpio.o
	$(CC) -o $@ $^

bl-server : server.o bl-server.o util.o simpio.o
	$(CC) -o $@ $^

simpio-demo : simpio-demo.o simpio.o
	$(CC) -o $@ $^

util.o : util.c blather.h
	$(CC) -c $<

simpio.o : simpio.c
	$(CC) -c $<

simpio-demo.o : simpio-demo.c
	$(CC) -c $<

server.o : server.c blather.h
	$(CC) -c $<

bl-server.o : bl-server.c blather.h
	$(CC) -c $<

bl-client.o : bl-client.c blather.h
	$(CC) -c $<

shell-tests : shell_tests.sh shell_tests_data.sh cat-sig.sh clean-tests
	chmod u+rx shell_tests.sh shell_tests_data.sh normalize.awk filter-semopen-bug.awk

clean-tests :
	rm -f *.out.expect *.valgrindout *.out
clean : 
	rm -f *.fifo *.o valgrind.txt simpio-demo *.log *.out *.out.expect *.diff *.valgrindout

