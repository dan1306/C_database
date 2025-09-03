# Targets
TARGET_SRV = bin/dbserver
TARGET_CLI = bin/dbcli

# Server sources and objects
SRC_SRV = $(wildcard src/srv/*.c)
OBJ_SRV = $(SRC_SRV:src/srv/%.c=obj/srv/%.o)

# Client sources and objects
SRC_CLI = $(wildcard src/cli/*.c)
OBJ_CLI = $(SRC_CLI:src/cli/%.c=obj/cli/%.o)

# Default target builds both
default: $(TARGET_SRV) $(TARGET_CLI)

# Run server
run: default
	./$(TARGET_SRV) -f ./mynewdb.db -n -p 8080

# Clean build artifacts
clean: 
	rm -rf obj/srv/*.o obj/cli/*.o
	rm -f bin/*
	rm -f *.db

# =====================
# Server Build
# =====================
$(TARGET_SRV): $(OBJ_SRV)
	gcc -o $@ $^

obj/srv/%.o: src/srv/%.c | obj/srv
	gcc -c $< -o $@ -Iinclude

obj/srv:
	mkdir -p obj/srv

# =====================
# Client Build
# =====================
$(TARGET_CLI): $(OBJ_CLI)
	gcc -o $@ $^

obj/cli/%.o: src/cli/%.c | obj/cli
	gcc -c $< -o $@ -Iinclude

obj/cli:
	mkdir -p obj/cli

