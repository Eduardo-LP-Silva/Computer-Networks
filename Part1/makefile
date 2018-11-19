CC		 = gcc
CFLAGS	 = -g
LDFLAGS  =
DEPS	 = main.c makefile
OBJFILES = main.o
TARGET	 = main

all: $(TARGET)

$(TARGET): $(OBJFILES) $(DEPS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LDFLAGS)

clean:
	rm -f $(OBJFILES) $(TARGET) *~
