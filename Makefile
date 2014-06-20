all:
	gcc -Wall -Wextra -pedantic -std=c99 -o event_count event_count.c

release:
	strip -x event_count

debug:
	gcc -g -Wall -Wextra -pedantic -std=c99 -o event_count event_count.c

clean:
	rm -f event_count

cleandata:
	rm -f event_count events.count
