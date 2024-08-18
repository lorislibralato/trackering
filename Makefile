CC = clang
BUILD_DIR = build

src_files = src/cache.c src/memory.c src/tracker.c src/io.c src/client.c src/tree.c c-rbtree/src/c-rbtree.c
obj_files = $(patsubst %.c, $(BUILD_DIR)/%.o,$(src_files)) 
INCLUDES = -I$(BUILD_DIR)/include/liburing/ -I$(BUILD_DIR)/include/
CFLAGS = -O3 -Wall -Wextra -march=native -ffunction-sections -flto $(INCLUDES)
LDFLAGS = -flto -fuse-ld=gold
LOADLIBES = -L$(BUILD_DIR)/lib

liburing = $(BUILD_DIR)/lib/liburing.a
server_obj = $(BUILD_DIR)/src/tracker_main.o
client_obj = $(BUILD_DIR)/src/client_main.o

server_bin = $(BUILD_DIR)/server
client_bin = $(BUILD_DIR)/client

.PHONY: build_server build_client clean

dir:
	@mkdir -p $(BUILD_DIR)/src

$(liburing):
	@$(MAKE) -j4 -C liburing install prefix=$(BUILD_DIR) CC=clang LIBURING_CFLAGS="-flto"

$(BUILD_DIR)/%.o: %.c
	@$(CC) $(CFLAGS) -c -o $@ $<

$(server_bin): $(obj_files) $(liburing) $(server_obj)
	@$(CC) $(LDFLAGS) $(LOADLIBES) $(LDLIBS) -o $@ $^ 

$(client_bin): $(obj_files) $(liburing) $(client_obj)
	@$(CC) $(LDFLAGS) $(LOADLIBES) $(LDLIBS) -o $@ $^ 

build_server: dir $(liburing) $(server_bin) 

build_client: dir $(liburing) $(client_bin)

clean_liburing:
	@$(MAKE) -C liburing clean prefix=$(BUILD_DIR)

clean: clean_liburing
	@rm -rf $(BUILD_DIR)
	@rm -rf $(main)