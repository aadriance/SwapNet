lib: SwapNet.o
	gcc -shared -o libSwapNet.so SwapNet.o -ldl

SwapNet.o: SwapNet.c
	gcc -c -Wall -Werror -fpic SwapNet.c -ldl

clean:
	rm *.o
	rm *.so
