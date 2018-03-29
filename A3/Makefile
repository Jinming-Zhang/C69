all: ext2_ls ext2_cp ext2_mkdir ext2_ln ext2_rm

ext2_ls : ext2_ls.c
	gcc -Wall -o ext2_ls ext2_ls.c

ext2_cp: ext2_cp.c
	gcc -Wall -o ext2_cp ext2_cp.c 

ext2_mkdir : ext2_mkdir.c
	gcc -Wall -o ext2_mkdir ext2_mkdir.c

ext2_ln: ext2_ln.c
	gcc -Wall -o ext2_ln ext2_ln.c

ext2_rm : ext2_rm.c
	gcc -Wall -o ext2_rm ext2_rm.c


clean:
	rm -f ext2_ls ext2_cp ext2_mkdir ext2_ln ext2_rm *.o
