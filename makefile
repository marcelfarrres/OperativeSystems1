ALL_FLAGS = -Wall -Wextra -pthread
NO_FLAGS = -Wall

b: 
	gcc bowman.c common.c -o bowman $(ALL_FLAGS)
	./bowman bowman.dat

p: 
	gcc poole.c common.c -o poole $(ALL_FLAGS)
	./poole poole.dat

d: 
	gcc discovery.c common.c -o discovery $(ALL_FLAGS)
	./discovery discovery.dat


