TARGET=./testmain

GCC=g++
SRCS=$(wildcard *.c)
OBJS=$(patsubst %.c, %.o, $(SRCS))

LIBS+= -lpthread

#thread safe
CFLAG+= -DUSELOCK  

ARCH=$(shell getconf LONG_BIT)
ifeq ($(ARCH), 64) 
CFLAG+= -DSYSBYTE=8
else
CFLAG+= -DSYSBYTE=4
endif

$(TARGET): $(OBJS)
	$(GCC) $^ -o $@   $(CFLAG) $(LIBS)	


clean:
	rm -rf $(TARGET)  *.o
