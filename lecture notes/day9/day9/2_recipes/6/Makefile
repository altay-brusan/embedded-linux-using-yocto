# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -g -Wall -DUSE_SYSCALL

# the name to use for both the target source file, and the output file:
TARGET = userprog

all: $(TARGET)

$(TARGET): $(TARGET).c
	${CC} $(CFLAGS) -o $(TARGET)  $(TARGET).c $(LDFLAGS)

install:
	install -d $(DESTDIR)
	install -m 0755 $(TARGET) $(DESTDIR)

uninstall:
	rm $(DESTDIR)$(TARGET)
