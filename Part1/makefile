CC		 = gcc
CFLAGS	 = -g
LDFLAGS  =
DEPS	 = main.c connection.c connection.h utilities.c utilities.h makefile
OBJFILES = main.o connection.o utilities.o
TARGET	 = download

all: $(TARGET)

$(TARGET): $(OBJFILES) $(DEPS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LDFLAGS)

clean:
	rm -f $(OBJFILES) $(TARGET) *~
