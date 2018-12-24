all:
	make ss
	make vanilla

vanilla:
	gcc -g -Wall --std=gnu99 -o vanilla main_vanilla.c -O2 -ldl -lm -llua -I../dependencies/luadata -L../dependencies/luadata -ldata

ss:
	gcc -g -Wall --std=gnu99 -o server main_ss.c -O2

clean:
	$(RM) server
	$(RM) vanilla
