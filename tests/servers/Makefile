all:
	$(MAKE) ulp
	$(MAKE) vanilla

vanilla:
	gcc -g -Wall --std=gnu99 -o vanilla vanilla.c -O2 -ldl -lm -llua -I../../dependencies/luadata -L../../dependencies/luadata -ldata

ulp:
	gcc -g -Wall --std=gnu99 -o ulp ulp.c -O2

clean:
	rm -rf vanilla ulp
