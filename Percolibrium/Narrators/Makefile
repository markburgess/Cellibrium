
all: stories stories2

stories: stories-fs.c ../Percolators/associations.c
	gcc -o stories -g stories-fs.c -lcrypto

stories2: stories-fs2.c context.c ../Percolators/associations.c
	gcc -o stories2 -g stories-fs2.c -lcrypto
