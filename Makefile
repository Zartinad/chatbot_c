PORT=52072
CFLAGS = -DPORT=\$(PORT) -g -Wall -std=gnu99
SERVER = socket.h

server: server.o socket.o general_functions.o
	gcc $(CFLAGS) -o $@ $^

# Separately compile each C file
%.o : %.c ${HDR}
		gcc ${CFLAGS} -c $<

clean:
	rm server *.o
