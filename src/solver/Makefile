solver: solver.c
	gcc -Wall -Wextra -std=c11 -o solver solver.c

show_solved_objects = show_solved.o solver.o
show_solved: $(show_solved_objects)
	gcc -Wall -Wextra -std=c11 -o show_solved $(show_solved_objects)

%.o: %.c
	gcc -Wall -Wextra -std=c11 -c $<

all: solver show_solved

clean:
	rm -f solver show_solved *.o
