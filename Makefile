# ==============================================================================
# SMART ENERGY CONSUMPTION TRACKER - MAKEFILE
# Desktop C Application using GTK+ 3.0 GUI and DSA Backend
# ==============================================================================

CC = gcc
PKG_CONFIG = pkg-config

CFLAGS = -Wall -Wextra -std=c99 -I. -O2 $(shell $(PKG_CONFIG) --cflags gtk+-3.0)
LDFLAGS = $(shell $(PKG_CONFIG) --libs gtk+-3.0) -lm

TARGET = SmartEnergyTracker.exe
SRCS = main.c energy.c
OBJS = main.o energy.o
HEADERS = energy.h

.PHONY: all run clean help

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "=========================================================="
	@echo " Build successful: $(TARGET)"
	@echo " Run with: ./$(TARGET) or 'make run'"
	@echo "=========================================================="

main.o: main.c $(HEADERS)
	$(CC) $(CFLAGS) -c main.c -o main.o

energy.o: energy.c $(HEADERS)
	$(CC) $(CFLAGS) -c energy.c -o energy.o

run: all
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
	@echo "Clean completed."

help:
	@echo "Available targets:"
	@echo "  make       - Compile desktop application with GTK3"
	@echo "  make run   - Compile and launch the application"
	@echo "  make clean - Remove compiled objects and executable"
