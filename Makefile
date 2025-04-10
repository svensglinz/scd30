CC := gcc
FLAGS := -O2
OUTPUT_DIR := ./bin
SRC_DIR := ./src
INCLUDE_DIR := ./include
OUTPUT := ./bin/query_scd30
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)

all: $(OUTPUT)

$(OUTPUT): $(SRC_FILES)
	@mkdir -p ./bin
	$(CC) $(FLAGS) $(SRC_FILES) -I$(INCLUDE_DIR) -o $@

# Clean up object and executable files
clean:
	rm -rf $(OUTPUT_DIR)
