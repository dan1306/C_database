TARGET_SRV = bin/dbserver
TARGET_CLI = bin/dbcli   
SRC = $(wildcard src/*.c)  # all `.c` files in `src/`
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC)) # Converts `.c` filenames to `.o` in `obj/`

SRC_CLI = $(wildcard SRC/CLI/*.c)
OBJ_CLI = $(SRC_CLI:src/cli/%.c=obj/cli/%.o)

run: clean default
	./$(TARGET) -f ./mynewdb.db -n -p 8080
	
default: $(TARGET_SRV) $(TARGET_CLI)

clean: 
	rm -rf obj/srv/*.o
	rm -f bin/*
	rm -f *.db

$(TARGET_SRV): $(OBJ_SRV)
	gcc -o $@ $?

$(OBJ_SRV): OBJ/SRV/%.o: src/srv/%.c
	gcc -c $< -o $@ -Iinclude

$(TARGET_CLI): $(OBJ_CLI)
	gcc -o $@ $?

$(OBJ_CLI): obj/cli/%.o: src/cli/%.c
	gcc -c $< -o $@ -Iinclude