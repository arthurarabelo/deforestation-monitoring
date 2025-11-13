CC       := gcc
CFLAGS   := -std=c11 -Wall -Wextra -g -Iinclude
LDFLAGS  :=
LDLIBS   := -pthread

SRC_DIR  := src
OBJ_DIR  := obj

COMMON_SRCS := $(SRC_DIR)/communication.c \
               $(SRC_DIR)/utils.c \
               $(SRC_DIR)/graph.c
SERVER_SRCS := $(SRC_DIR)/server.c
CLIENT_SRCS := $(SRC_DIR)/client.c

COMMON_OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(COMMON_SRCS))
SERVER_OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SERVER_SRCS)) $(COMMON_OBJS)
CLIENT_OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(CLIENT_SRCS)) $(COMMON_OBJS)

TARGETS := server client

.PHONY: all clean

all: $(TARGETS)

server: $(SERVER_OBJS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

client: $(CLIENT_OBJS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) $(LDLIBS) -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	@mkdir -p $@

clean:
	rm -rf $(OBJ_DIR) server client
