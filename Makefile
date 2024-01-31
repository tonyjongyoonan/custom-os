# Directories
SRC_DIR := src
OBJ_DIR := obj

# Find all source files in the source directory
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
HEADERS := $(wildcard $(SRC_DIR)/*.h)

# Generate a list of object files in the obj directory
OBJ_FILES := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC_FILES))

PROG=PennOS
PROMPT='"$(PROG)> "'
override CPPFLAGS += -DNDEBUG -DPROMPT=$(PROMPT)
# Compiler and flags
CC := clang
CFLAGS := -Wall -Werror -g -Wno-unused-function -Wno-unused-variable
CPPFLAGS := -I$(SRC_DIR)

# Targets
all: $(OBJ_FILES) 
	clang -o $(PROG) $(OBJ_FILES) obj/parser.o
	mv PennOS bin/


# Rule to compile .c files to .o files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

# # Create the obj directory if it doesn't exist
# $(OBJ_DIR):
# 	mkdir -p $(OBJ_DIR)

# $(PROG) : $(OBJ_FILES)
# 	$(CC) -o $@ $(OBJ_FILES) obj/parser.o

# Define which object files to exclude from clean
EXCLUDED_OBJECTS := obj/parser.o

clean:
	rm -rf $(filter-out $(EXCLUDED_OBJECTS), $(OBJ_FILES))

.PHONY: all clean
