LUNATIK := dependencies/lunatik/lua
LUNATIKARCH := dependencies/lunatik/arch

subdir-ccflags-y := -I${PWD}/${LUNATIK} \
	-I${PWD}/${LUNATIKUTIL} \
	-Wall \
	-D_KERNEL \
	-D'CHAR_BIT=(8)' \
	-D'MIN=min' \
	-D'MAX=max' \
	-D'UCHAR_MAX=(255)' \
	-D'UINT64_MAX=((u64)~0ULL)'

ss-objs := src/module.o src/allocator.o \
      ${LUNATIK}/lapi.o ${LUNATIK}/lcode.o \
      ${LUNATIK}/lctype.o ${LUNATIK}/ldebug.o \
      ${LUNATIK}/ldo.o ${LUNATIK}/ldump.o \
      ${LUNATIK}/lfunc.o ${LUNATIK}/lgc.o \
      ${LUNATIK}/llex.o ${LUNATIK}/lmem.o \
      ${LUNATIK}/lobject.o ${LUNATIK}/lopcodes.o \
      ${LUNATIK}/lparser.o ${LUNATIK}/lstate.o \
      ${LUNATIK}/lstring.o ${LUNATIK}/ltable.o \
      ${LUNATIK}/ltm.o ${LUNATIK}/lundump.o \
      ${LUNATIK}/lvm.o ${LUNATIK}/lzio.o \
      ${LUNATIK}/lauxlib.o ${LUNATIK}/lbaselib.o \
      ${LUNATIK}/lbitlib.o ${LUNATIK}/lcorolib.o \
      ${LUNATIK}/ldblib.o ${LUNATIK}/lstrlib.o \
      ${LUNATIK}/ltablib.o ${LUNATIK}/lutf8lib.o \
      ${LUNATIK}/loslib.o ${LUNATIK}/lmathlib.o \
      ${LUNATIK}/linit.o \
      ${LUNATIKARCH}/x86_64/setjmp.o

obj-m += ss.o dependencies/luadata/
