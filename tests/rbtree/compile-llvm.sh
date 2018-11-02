#llvm-gcc -c -I"/usr/local/gasnet/include" -I"/usr/local/gasnet/include/ibv-conduit" -O4 --emit-llvm --gnu-tm pthread1.c -o test.bc

#llvm-gcc -c -I"/sfsdata/home/danilov/bin/berkeley_upc/include/" -I"/sfsdata/home/danilov/bin/berkeley_upc/include/ibv-conduit" -O4 --emit-llvm --gnu-tm -DGASNET_PAR -DGASNET_CONDUIT_IBV=1 intset_my.c -o intset.bc
llvm-gcc -c -I"/home/zkaliaev/danilov/install/include" -I"/home/zkaliaev/danilov/install/include/ibv-conduit" -O4 --emit-llvm --gnu-tm -DGASNET_PAR -DGASNET_CONDUIT_IBV=1 rbtree_my.c -o rbtree.bc
mv rbtree.bc /home/zkaliaev/PROJECTS
#llvm-gcc -c -I"/usr/local/gasnet/include" -I"/usr/local/gasnet/include/ibv-conduit" -O4 --emit-llvm dstm_barrier.c -o dstm_barrier.bc 
#llvm-link -o test.bc pthread1.bc dstm_barrier.bc
