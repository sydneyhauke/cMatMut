CC=gcc

CFLAGS= -Wall -O2 -g
LDFLAGS= -pthread -lm -lOpenCL

EXEC=matmut
BUILD_DIR=build
SRC_DIR=src
SRC=$(wildcard $(SRC_DIR)/*.c)
OBJ=$(SRC:.c=.o)
OBJ += $(SRC_DIR)/optim.o

.PHONY: clean mrproper

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) -o $(BUILD_DIR)/$@ $^ $(LDFLAGS)

$(SRC_DIR)/optim.o:
	$(AS) -g -o $@ $(SRC_DIR)/optim.S

$(SRC_DIR)/%.o: %.c
	$(CC) -o $@ -c $< $(CFLAGS)

clean:
	rm -rf $(SRC_DIR)/*.o

mrproper: clean
	rm -rf $(BUILD_DIR)/*
