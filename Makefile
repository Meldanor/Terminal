SERVER_NAME = TerminalServer
SERVER_VERSION = 1.0
SERVER_DIR = server/
SERVER_FILES = *.c

COMMON_FILES = network/* common/*

OUT_DIR = ./bin

all: directories $(SERVER_NAME)

directories:
	mkdir -p $(OUT_DIR)

$(SERVER_NAME): $(SERVER_DIR)$(SERVER_FILES)
	gcc -pthread -Wall -o $(OUT_DIR)/$(SERVER_NAME)-$(SERVER_VERSION) $(COMMON_FILES) $(SERVER_DIR)$(SERVER_FILES) 
