CC=gcc
CFLAGS=-Wall -O2
LDFLAGS=
SOURCES=main.c base.c clientlist.c networking.c resources.c
OBJECTS=${SOURCES:.c=.o}

cwebserver: ${OBJECTS}
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CLAGS) -o $@ -c $<

clean:
	rm -r *.o cwebserver
