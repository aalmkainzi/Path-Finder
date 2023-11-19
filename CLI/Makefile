debug: src/main.c src/path_finder.c src/priority_queue.c
	gcc -ggdb -fsanitize=address src/main.c src/path_finder.c src/priority_queue.c -o bin/path -Wall -Wextra
path: src/main.c src/path_finder.c src/priority_queue.c
	gcc -O3 -flto src/main.c src/path_finder.c src/priority_queue.c -o bin/path -Wall -Wextra