ALL_FLAGS = -Wall -Wextra -pthread
NO_FLAGS = -Wall

b1: 
	gcc -g bowman.c common.c -o bowman $(ALL_FLAGS)
	./bowman data/bowman1.dat

b2: 
	gcc -g bowman.c common.c -o bowman $(ALL_FLAGS)
	./bowman data/bowman2.dat

b3: 
	gcc -g bowman.c common.c -o bowman $(ALL_FLAGS)
	./bowman data/bowman3.dat

b4: 
	gcc -g bowman.c common.c -o bowman $(ALL_FLAGS)
	./bowman data/bowman4.dat

p1: 
	gcc -g poole.c common.c -o poole $(ALL_FLAGS)
	./poole data/poole1.dat

p2: 
	gcc -g poole.c common.c -o poole $(ALL_FLAGS)
	./poole data/poole2.dat

p3: 
	gcc -g poole.c common.c -o poole $(ALL_FLAGS)
	./poole data/poole3.dat

d: 
	gcc -g discovery.c common.c -o discovery $(ALL_FLAGS)
	./discovery data/discovery.dat






vb1: 
	gcc -g bowman.c common.c -o bowman $(ALL_FLAGS)
	valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes ./bowman data/bowman1.dat

vb2: 
	gcc -g bowman.c common.c -o bowman $(ALL_FLAGS)
	valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes ./bowman data/bowman2.dat

vb3: 
	gcc -g bowman.c common.c -o bowman $(ALL_FLAGS)
	valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes ./bowman data/bowman3.dat	

vb4: 
	gcc -g bowman.c common.c -o bowman $(ALL_FLAGS)
	valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes ./bowman data/bowman4.dat	

vp1: 
	gcc -g poole.c common.c -o poole $(ALL_FLAGS)
	valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes ./poole data/poole1.dat

vp2: 
	gcc -g poole.c common.c -o poole $(ALL_FLAGS)
	valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes ./poole data/poole2.dat

vp3: 
	gcc -g poole.c common.c -o poole $(ALL_FLAGS)
	valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes ./poole data/poole3.dat

vd: 
	gcc -g discovery.c common.c -o discovery $(ALL_FLAGS)
	valgrind -s --leak-check=full --show-leak-kinds=all --track-origins=yes ./discovery data/discovery.dat


