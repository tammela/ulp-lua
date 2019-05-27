all:
	$(MAKE) ulp
	$(MAKE) vanilla

vanilla:
	gcc -g -Wall --std=gnu99 -o vanilla.out vanilla.c -O2 -ldl -lm -llua -I../../dependencies/luadata -L../../dependencies/luadata -ldata
	gcc -g -Wall --std=gnu99 -o vanilla6.out vanilla6.c -O2 -ldl -lm -llua -I../../dependencies/luadata -L../../dependencies/luadata -ldata

ulp:
	gcc -g -Wall --std=gnu99 -o ulp.out ulp.c -O2
	gcc -g -Wall --std=gnu99 -o ulp6.out ulp6.c -O2

clean:
	rm -rf *.out
