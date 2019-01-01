all:
	make -C /lib/modules/${shell uname -r}/build M=${PWD} \
	CONFIG_LUADATA=m \
	CONFIG_LUNATIK=m

clean:
	make -C /lib/modules/${shell uname -r}/build M=${PWD} clean
