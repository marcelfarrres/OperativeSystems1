ALL_FLAGS = -Wall -Wextra -pthread
NO_FLAGS = -Wall

b: 
	gcc -g bowman.c common.c -o bowman $(ALL_FLAGS)
	./bowman bowman.dat

p: 
	gcc -g poole.c common.c -o poole $(ALL_FLAGS)
	./poole poole.dat

d: 
	gcc -g discovery.c common.c -o discovery $(ALL_FLAGS)
	./discovery discovery.dat


vb:
	gcc -g bowman.c common.c -o bowman $(ALL_FLAGS)
	valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes ./bowman bowman.dat

vp:
	gcc -g poole.c common.c -o poole $(ALL_FLAGS)
	valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes ./poole poole.dat

vd:
	gcc -g discovery.c common.c -o discovery $(ALL_FLAGS)
	valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes ./discovery discovery.dat
