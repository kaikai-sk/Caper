#DISKMODEL_PREFIX = /home/li/ns103pm/lib/diskmodel
#PARAM_PREFIX = /home/li/ns103pm/lib/libparam
#DDBG_PREFIX = /home/li/ns103pm/lib/libddbg

DISKMODEL_CFLAGS = -I/home/li/disksim-4.0-ssd/diskmodel/include
LIBPARAM_CFLAGS = -I/home/li/disksim-4.0-ssd/libparam/include
LIBDDBG_CFLAGS = -I/home/li/disksim-4.0-ssd/libddbg/include
MEMSMODEL_CFLAGS = -I/home/li/disksim-4.0-ssd/memsmodel/include
DISKSIM_CFLAGS = -I/home/li/disksim-4.0-ssd/src
#DISKSIM_CFLAGS = -I/home/li/disksim-4.0/include

DISKMODEL_LDFLAGS = -L/home/li/disksim-4.0-ssd/diskmodel/lib -ldiskmodel
LIBPARAM_LDFLAGS = -L/home/li/disksim-4.0-ssd/libparam/lib -lparam
LIBDDBG_LDFLAGS = -L/home/li/disksim-4.0-ssd/libddbg/lib -lddbg
MEMSMODEL_LDFLAGS = -L/home/li/disksim-4.0-ssd/memsmodel/lib -lmemsmodel
DISKSIM_LDFLAGS = -L/home/li/disksim-4.0-ssd/src -ldisksim
#DISKSIM_LDFLAGS = -L/home/li/disksim-4.0/lib -ldisksim

#$(CC) $(CFLAGS) -c syssim_driver.cpp $(LDFLAGS)

CC = g++ -Wall -g
CFLAGS = -I. $(DISKMODEL_CFLAGS) $(LIBPARAM_CFLAGS) $(LIBDDBG_CFLAGS) $(MEMSMODEL_CFLAGS) $(DISKSIM_CFLAGS) 
LDFLAGS = -lm $(DISKMODEL_LDFLAGS) $(MEMSMODEL_LDFLAGS) $(LIBPARAM_LDFLAGS) $(LIBDDBG_LDFLAGS) 


forney: forney.o application.o buflist.o hash.o
	$(CC) $(CFLAGS) -o forney forney.o application.o buflist.o hash.o /home/li/disksim-4.0-ssd/src/disksim_interface.o  /home/li/disksim-4.0-ssd/src/libdisksim.a /home/li/disksim-4.0-ssd/ssdmodel/libssdmodel.a $(LDFLAGS)

forney.o: forney.cpp forney.h
	$(CC) $(CFLAGS) -c forney.cpp -o forney.o $(LDFLAGS)
application.o: application.cpp application.h
	$(CC) -c application.cpp -o application.o
buflist.o: buflist.cpp buflist.h
	$(CC) -c buflist.cpp -o buflist.o
hash.o: hash.cpp hash.h
	$(CC) -c hash.cpp -o hash.o

clean:
	cp forney ../test/forney-prefetch
	rm forney *.o

