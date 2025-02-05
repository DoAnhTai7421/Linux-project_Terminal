.PHONY: all clean

all:
	gcc -o Terminal main.c -lreadline

clean:
	rm -rf Terminal
