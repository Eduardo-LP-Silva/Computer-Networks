CC		 = gcc
CFLAGS	 = -g
LDFLAGS  =
DEPS	 = main.c connection.c connection.h makefile
OBJFILES = main.o connection.o
TARGET	 = download

all: $(TARGET)

$(TARGET): $(OBJFILES) $(DEPS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LDFLAGS)

clean:
	rm -f $(OBJFILES) $(TARGET) *~
