.SUFFIXES: .cpp .o

CC=g++

SRCDIR=src/
INC=include/
LIBS=lib/

# SRCS:=$(wildcard src/*.c)
# OBJS:=$(SRCS:.c=.o)

# main source file
TARGET_SRC:=$(SRCDIR)main.cpp
TARGET_OBJ:=$(SRCDIR)main.o

# Include more files if you write another source file.
SRCS_FOR_LIB:=$(SRCDIR)bpt.cpp $(SRCDIR)buffer.cpp $(SRCDIR)buffer_structure.cpp $(SRCDIR)file_manager.cpp $(SRCDIR)transaction.cpp $(SRCDIR)log.cpp
OBJS_FOR_LIB:=$(SRCS_FOR_LIB:.cpp=.o)

CFLAGS+= -g -fPIC -I $(INC)

TARGET=main

all: $(TARGET)

$(TARGET): $(TARGET_SRC)
	#$(foreach file, $(SRCS_FOR_LIB), $(CC) $(CFLAGS) -o $(file:.c = .o) -c $(file);)
	#$(CC) $(CFLAGS) -o $(OBJS_FOR_LIB) -c $(SRCS_FOR_LIB)
	$(CC) $(CFLAGS) -o src/bpt.o -c src/bpt.cpp -pthread
	$(CC) $(CFLAGS) -o src/buffer.o -c src/buffer.cpp -pthread
	$(CC) $(CFLAGS) -o src/transaction.o -c src/transaction.cpp -pthread
	$(CC) $(CFLAGS) -o src/log.o -c src/log.cpp -pthread
	$(CC) $(CFLAGS) -o src/buffer_structure.o -c src/buffer_structure.cpp 
	$(CC) $(CFLAGS) -o src/file_manager.o -c src/file_manager.cpp
	make static_library
	$(CC) $(CFLAGS) -o $@ $^ -L $(LIBS) -lbpt -pthread

clean:
	rm $(TARGET) $(OBJS_FOR_LIB) $(LIBS)*

library:
	g++ -shared -Wl,-soname,libbpt.so -o $(LIBS)libbpt.so $(OBJS_FOR_LIB)

static_library:
	mkdir -p $(LIBS)
	ar cr $(LIBS)libbpt.a $(OBJS_FOR_LIB)

