#DISKMODEL_PREFIX =/home/shankai/disksim-4.0/diskmodel
#PARAM_PREFIX = /home/shankai/disksim-4.0/libparam
#DDBG_PREFIX = /home/shankai/disksim-4.0/libddbg

pythonVersion=python2.6

DISKMODEL_CFLAGS = -I/home/shankai/disksim-4.0/diskmodel/include
LIBPARAM_CFLAGS = -I/home/shankai/disksim-4.0/libparam/include
LIBDDBG_CFLAGS = -I/home/shankai/disksim-4.0/libddbg/include
MEMSMODEL_CFLAGS = -I/home/shankai/disksim-4.0/memsmodel/include
DISKSIM_CFLAGS = -I//home/shankai/disksim-4.0/src
PYTHON_CFLAGS = -I/usr/include/$(pythonVersion)
#DISKSIM_CFLAGS = -I/home/shankai/disksim-4.0/include

DISKMODEL_LDFLAGS = -L/home/shankai/disksim-4.0/diskmodel/lib -ldiskmodel
LIBPARAM_LDFLAGS = -L/home/shankai/disksim-4.0/libparam/lib -lparam
LIBDDBG_LDFLAGS = -L/home/shankai/disksim-4.0/libddbg/lib -lddbg
MEMSMODEL_LDFLAGS = -L/home/shankai/disksim-4.0/memsmodel/lib -lmemsmodel
DISKSIM_LDFLAGS = -L/home/shankai/disksim-4.0/src -ldisksim
PYTHON_LDFLAGS = -L/usr/lib/$(pythonVersion) -lpython2.6 
#DISKSIM_LDFLAGS = -L/home/shankai/disksim-4.0/lib -ldisksim

#$(CC) $(CFLAGS) -c syssim_driver.cpp $(LDFLAGS)

CC = g++ -Wall -g
CFLAGS = -I. $(DISKMODEL_CFLAGS) $(LIBPARAM_CFLAGS) $(LIBDDBG_CFLAGS) $(MEMSMODEL_CFLAGS) $(DISKSIM_CFLAGS) $(PYTHON_CFLAGS) 
LDFLAGS = -lm $(DISKMODEL_LDFLAGS) $(MEMSMODEL_LDFLAGS) $(LIBPARAM_LDFLAGS) $(LIBDDBG_LDFLAGS) $(PYTHON_LDFLAGS)

main:main.o
	$(CC) $(CFLAGS) -o main main.o /home/shankai/disksim-4.0/src/disksim_interface.o  /home/shankai/disksim-4.0/src/libdisksim.a /home/shankai/disksim-4.0/ssdmodel/libssdmodel.a $(LDFLAGS)
main.o:main.cpp
	$(CC) $(CFLAGS) -c main.cpp -o main.o $(LDFLAGS)

clean:
	rm main *.o
