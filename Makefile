BUILD = build
CC = gcc
LIBS = -ldl
EXTRA ?= 
CFLAGS = -Wall -Wextra -c $(EXTRA) -O3
CF2 = $(EXTRA) -O3 -flto
AR = ar
ARFLAGS = rcs
SRC = src
TARGET_LIB = lib/mylib.so
LIB_SRC = lib/mylib.c
MAIN_OBJ = $(BUILD)/zynk.o
TARGET = $(BUILD)/zynk
OBJS = $(BUILD)/mbtype.o $(BUILD)/parser.o $(BUILD)/lexer.o $(BUILD)/eval.o $(BUILD)/globals.o

all: $(TARGET)
lib: $(TARGET_LIB)
$(TARGET): $(OBJS) $(MAIN_OBJ)
	$(CC) $(CF2) -rdynamic $(OBJS) $(MAIN_OBJ) $(LIBS) -o $(TARGET)

# Regla para compilar el plugin dinámico a .so
$(TARGET_LIB): $(LIB_SRC)
	$(CC) -shared -fPIC -Wall -Wextra $(EXTRA) -o $@ $(LIB_SRC)

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
