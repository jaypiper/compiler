SRC_DIR  = ./src
BUILD_DIR = ./build
TEST_DIR = ./testcase

TEST ?= test
INCLUDE_DIR = ./include $(BUILD_DIR)
SYNTAX_SRC = $(SRC_DIR)/syntax.y
LEXICAL_SRC = $(SRC_DIR)/lexical.l

SYNTAX_OUTPUT = $(BUILD_DIR)/syntax.c
LEXICAL_OUTPUT = $(BUILD_DIR)/lexical.c
OUTPUT = $(BUILD_DIR)/parser

SRCS = $(shell find $(SRC_DIR) -name "*.c") $(LEXICAL_OUTPUT) $(SYNTAX_OUTPUT)

CFLAGS = $(addprefix -I,$(INCLUDE_DIR))
CFLAGS += -lfl -ggdb

LINK_ADDR = 0x80000000
TEST_CFLAGS = -nostartfiles -static -T scripts/section.ld
TEST_LDFLAGS += -Wl,-Ttext-segment=$(LINK_ADDR) -Wl,--build-id=none
CROSS_COMPILE = riscv64-linux-gnu-
CC 				= $(CROSS_COMPILE)gcc
OBJDUMP   = $(CROSS_COMPILE)objdump
OBJCOPY   = $(CROSS_COMPILE)objcopy

parser: syntax lexical
	gcc $(SRCS) $(CFLAGS) -o $(OUTPUT)

syntax: $(SYNTAX_SRC)
	mkdir -p $(BUILD_DIR)
	bison -d $(SYNTAX_SRC) -o $(SYNTAX_OUTPUT)

lexical: $(LEXICAL_SRC) syntax
	flex -o $(LEXICAL_OUTPUT) $(LEXICAL_SRC)

test: parser
	$(OUTPUT) $(TEST_DIR)/$(TEST).c $(BUILD_DIR)/$(TEST)-inter.out $(BUILD_DIR)/$(TEST)-riscv.S

elf: test
	$(CC) $(BUILD_DIR)/$(TEST)-riscv.S $(TEST_CFLAGS) $(TEST_LDFLAGS) -o $(BUILD_DIR)/$(TEST)-riscv.elf
	$(OBJCOPY) -S --set-section-flags .bss=alloc,contents -O binary $(BUILD_DIR)/$(TEST)-riscv.elf $(BUILD_DIR)/$(TEST)-riscv.bin
	$(OBJDUMP) -d $(BUILD_DIR)/$(TEST)-riscv.elf > $(BUILD_DIR)/$(TEST)-riscv.txt

clean:
	rm -rf $(BUILD_DIR)
