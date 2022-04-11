SRC_DIR  = ./src
BUILD_DIR = ./build
INCLUDE_DIR = ./include $(BUILD_DIR)
SYNTAX_SRC = $(SRC_DIR)/syntax.y
LEXICAL_SRC = $(SRC_DIR)/lexical.l

SYNTAX_OUTPUT = $(BUILD_DIR)/syntax.c
LEXICAL_OUTPUT = $(BUILD_DIR)/lexical.c
OUTPUT = $(BUILD_DIR)/parser

SRCS = $(shell find $(SRC_DIR) -name "*.c") $(LEXICAL_OUTPUT) $(SYNTAX_OUTPUT)

CFLAGS = $(addprefix -I,$(INCLUDE_DIR))
CFLAGS += -lfl

parser: lexical syntax
	gcc $(SRCS) $(CFLAGS) -o $(OUTPUT)

syntax: $(SYNTAX_SRC)
	mkdir -p $(BUILD_DIR)
	bison -d $(SYNTAX_SRC) -o $(SYNTAX_OUTPUT)

lexical: $(LEXICAL_SRC) syntax
	flex -o $(LEXICAL_OUTPUT) $(LEXICAL_SRC)

clean:
	rm -rf $(BUILD_DIR)
