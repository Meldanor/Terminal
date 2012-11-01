CLIENT_NAME = TerminalClient
CLIENT_VERSION = 0.0.1-SNAPSHOT
CLIENT_DIR = client/
CLIENT_FILES = *.c

SERVER_NAME = TerminalServer
SERVER_VERSION = 0.0.1-SNAPSHOT
SERVER_DIR = server/
SERVER_FILES = *.c

COMMON_FILES = network/*.c

OUT_DIR = ./bin

all: directories $(CLIENT_NAME) $(SERVER_NAME)

directories:
	mkdir -p $(OUT_DIR)

$(CLIENT_NAME): $(CLIENT_DIR)$(CLIENT_FILES)
	gcc -pthread -o $(OUT_DIR)/$(CLIENT_NAME)-$(CLIENT_VERSION) $(COMMON_FILES) $(CLIENT_DIR)$(CLIENT_FILES)

$(SERVER_NAME): $(SERVER_DIR)$(SERVER_FILES)
	gcc -pthread -o $(OUT_DIR)/$(SERVER_NAME)-$(SERVER_VERSION) $(COMMON_FILES) $(SERVER_DIR)$(SERVER_FILES) 
