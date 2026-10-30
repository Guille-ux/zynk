BUILD = build
OS ?= linux
CC ?= gcc
LIBS ?= -ldl
EXTRA ?= 
CFLAGS = -Wall -Wextra -c $(EXTRA) -O3
CF2 = $(EXTRA) -O3 -flto -rdynamic
AR = ar
ARFLAGS = rcs
SRC = src
TARGET_LIB = lib/mylib.so
LIB_SRC = lib/mylib.c
MAIN_OBJ = $(BUILD)/zynk.o
TARGET = $(BUILD)/zynk
OBJS = $(BUILD)/mbtype.o $(BUILD)/parser.o $(BUILD)/lexer.o $(BUILD)/eval.o $(BUILD)/globals.o $(BUILD)/chunk.o $(BUILD)/vm.o $(BUILD)/compiler.o
ifeq ($(OS),windows)
	CC = x86_64-w64-mingw32-gcc
	CFLAGS = -D_WIN32 -Wall -Wextra -c $(EXTRA) -O3
	CF2 = $(EXTRA) -O3 -flto
	# Añadimos las librerías necesarias de la API de Windows para que el linker encuentre LoadLibraryA, etc.
	LIBS = -lkernel32 -luser32
	TARGET_LIB = lib/mylib.dll
	TARGET = $(BUILD)/zynk.exe
else
	CC = gcc
	CFLAGS = -D_LINUX -Wall -Wextra -c $(EXTRA) -O3
	#CFLAGS = -D_LINUX -Wall -Wextra -c $(EXTRA) -O0 -march=armv6zk -mfloat-abi=hard -mfpu=vfp
	LIBS = -ldl
	TARGET_LIB = lib/mylib.so
	TARGET = $(BUILD)/zynk
endif

all: folders $(TARGET)
lib: $(TARGET_LIB)
folders:
	mkdir -p build


$(TARGET): $(OBJS) $(MAIN_OBJ)
	$(CC) $(CF2) $(OBJS) $(MAIN_OBJ) $(LIBS) -o $(TARGET)

# Regla para compilar el plugin dinámico a .so
$(TARGET_LIB): $(LIB_SRC)
	$(CC) -shared -fPIC -Wall -Wextra $(EXTRA) -o $@ $(LIB_SRC)

$(BUILD)/compiler.o: $(SRC)/compiler.c
	$(CC) $(CFLAGS) -o $(BUILD)/compiler.o $(SRC)/compiler.c

$(BUILD)/vm.o: $(SRC)/vm.c
	$(CC) $(CFLAGS) -o $(BUILD)/vm.o $(SRC)/vm.c

$(BUILD)/chunk.o: $(SRC)/chunk.c
	$(CC) $(CFLAGS) -o $(BUILD)/chunk.o $(SRC)/chunk.c

$(BUILD)/globals.o: $(SRC)/globals.c
	$(CC) $(CFLAGS) -o $(BUILD)/globals.o $(SRC)/globals.c

$(BUILD)/libzynk.a: $(OBJS)
	$(AR) $(ARFLAGS) $(BUILD)/libzynk.a $(OBJS)

$(BUILD)/mbtype.o: $(SRC)/mbtype.c
	$(CC) $(CFLAGS) -o $(BUILD)/mbtype.o $(SRC)/mbtype.c

$(BUILD)/parser.o: $(SRC)/parser.c
	$(CC) $(CFLAGS) -o $(BUILD)/parser.o $(SRC)/parser.c

$(BUILD)/lexer.o: $(SRC)/lexer.c
	$(CC) $(CFLAGS) -o $(BUILD)/lexer.o $(SRC)/lexer.c

$(BUILD)/eval.o: $(SRC)/eval.c
	$(CC) $(CFLAGS) -o $(BUILD)/eval.o $(SRC)/eval.c

$(MAIN_OBJ): $(SRC)/zynk.c
	$(CC) $(CFLAGS) -o $(MAIN_OBJ) $(SRC)/zynk.c

.PHONY: clean

clean: 
	rm -rf build/*
	rm -rf lib/*.so
	rm -rf *.o
	rm -rf $(BUILD)/libzynk.a
