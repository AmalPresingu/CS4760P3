CC = gcc
CFLAGS = -I -g
OBJECT1 = master.o
OBJECT2 = slave.o
TARGET1 = master
TARGET2 = slave
CONFIG = config.h

PROCESS1 = master.c 
PROCESS2 = slave.c

all: $(PROCESS1) $(TARGET2) $(CONFIG)
	$(CC) $(CFLAGS) $(PROCESS1) -o $(TARGET1) 

slave: $(PROCESS2) $(CONFIG)
	$(CC) $(CFLAGS) $(PROCESS2) -o $(TARGET2)

%.o: %.c $(CONFIG)
	$(CC) $(CFLAGS) -c $@ $<

clean: 
	@rm -f *.o master slave logfile.* cstest
