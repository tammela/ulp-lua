LUADATA := dependencies/luadata
LUNATIK := dependencies/lunatik-hidden/lua

subdir-ccflags-y := -I${PWD}/${LUNATIK} \
	-I${PWD}/${LUADATA} \
	-Wall \
	-D_KERNEL \
        -D_MODULE \
	-D'CHAR_BIT=(8)' \
	-D'MIN=min' \
	-D'MAX=max' \
	-D'UCHAR_MAX=(255)' \
	-D'UINT64_MAX=((u64)~0ULL)'

obj-y += dependencies/lunatik-hidden/ dependencies/luadata/ src/
