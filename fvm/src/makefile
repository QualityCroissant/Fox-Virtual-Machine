CC=gcc

CFLAGS=-Wall -Wextra -O3

FVMR_LDFLAGS=-lGL -lglfw

FVMA_BIN_NAME=../fvma
FVMA_SRC_NAME=fvm_assembler.c

FVMR_BIN_NAME=../fvmr
FVMR_SRC_NAME=fvm_runtime.c

fvma:
	$(CC) $(CFLAGS) $(FVMA_SRC_NAME) -o $(FVMA_BIN_NAME)

fvmr:
	$(CC) $(CFLAGS) $(FVMR_SRC_NAME) $(FVMR_LDFLAGS) -o $(FVMR_BIN_NAME)
