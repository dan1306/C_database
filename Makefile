TARGET = bin/dbview  # executable located in `bin/dbview` 
SRC = $(wildcard src/*.c)  # all `.c` files in `src/`
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC)) # Converts `.c` filenames to `.o` in `obj/`

run: clean default
	./$(TARGET) -f ./mynewdb.db -n 
# 	./$(TARGET) -f ./mynewdb.db -n 
	
default: $(TARGET)

clean: 
	rm -rf obj/*
	rm -f bin/*
	rm -f *.db

$(TARGET): $(OBJ)
	gcc -o $@ $?

obj/%.o: src/%.c
	gcc -c -o $@ $< -Iinclude