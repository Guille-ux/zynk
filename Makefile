# El makefile para construir la libreria de zynk

BUILD = build
CC = gcc
LIBS = 
EXTRA ?= 
CFLAGS = -Wall -Wextra -c $(EXTRA)
AR = ar
ARFLAGS = rcs
SRC = src
MAIN_OBJ = $(BUILD)/zynk.o
TARGET = $(BUILD)/zynk
OBJS = $(BUILD)/mbtype.o $(BUILD)/parser.o $(BUILD)/lexer.o $(BUILD)/eval.o $(BUILD)/globals.o
$(TARGET): $(OBJS) $(MAIN_OBJ)
	$(CC) $(LIBS) -o $(TARGET) $(OBJS) $(MAIN_OBJ)

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
	rm -rf *.o
	rm -rf libmbash.a
