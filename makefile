GCC		= g++
MAKE		= make
OPTIMIZE	= -O2 -DSUPPORT_LH7 -DMKSTEMP
CFLAGS		= -std=c++11 -Winline -pipe -g
LDLIBS		= -lpthread -lUseful

Socket.so : socket.o crc.o
	#$(GCC) $(CFLAGS) -fPIC -shared Socket.cpp crc.o -o libSocket.so.1 $(LDLIBS)
	$(GCC) $(CFLAGS) -fPIC -shared socket.o crc.o -o libSocket.so.1 $(LDLIBS)
	mv libSocket.so.1 lib/

socket.o : Socket.cpp Socket.h
	$(GCC) -c $(CFLAGS) -fPIC Socket.cpp -o socket.o $(LDLIBS)

crc.o : crc.c crc.h
	$(GCC) -c $(CFLAGS) -fPIC crc.c -o crc.o $(LDLIBS)

tests : test.cpp server.cpp client.cpp
	$(GCC) $(CFLAGS) test.cpp -o test $(LDLIBS) -lSocket
	$(GCC) $(CFLAGS) server.cpp -o server $(LDLIBS) -lSocket
	$(GCC) $(CFLAGS) client.cpp -o client $(LDLIBS) -lSocket

install :
	scp Socket.h /usr/include/
	scp lib/libSocket.so.1 /usr/local/lib/libSocket.so
	chmod +x /usr/local/lib/libSocket.so
	ldconfig

uninstall :
	rm /usr/include/Socket.h
	rm /usr/local/lib/libSocket.so

clean :
	rm socket.o
	rm crc.o
	rm lib/libSocket.so.1
