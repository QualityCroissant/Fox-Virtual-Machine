CC=gcc

CFLAGS=-Wall -Wextra -O3

FVMR_LDFLAGS=-lGL -lglfw

FVMA_BIN_NAME=../fvma
FVMA_SRC_NAME=fvm_assembler.c
FVMA_COMPONENTS=fvm_assembler_components/*.c

FVMR_BIN_NAME=../fvmr
FVMR_SRC_NAME=fvm_runtime.c
FVMR_COMPONENTS=fvm_runtime_components/*.c

fvma:
	$(CC) $(CFLAGS) $(FVMA_COMPONENTS) $(FVMA_SRC_NAME) -o $(FVMA_BIN_NAME)

fvmr:
	$(CC) $(CFLAGS) $(FVMR_COMPONENTS) $(FVMR_SRC_NAME) $(FVMR_LDFLAGS) -o $(FVMR_BIN_NAME)
