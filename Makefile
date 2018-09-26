include Makefile.header

QEMU		:= qemu-system-i386
QEMUOPT		:= -hda kernel.img -serial stdio -gdb tcp::23456

all:
	make -C boot
	make -C lib
	make -C user
	make -C kernel
	cp tools/mkfs.py .
	python mkfs.py kernel.img
	rm -f mkfs.py

qemu: all
	$(QEMU) $(QEMUOPT)

qemu-gdb: all
	$(QEMU) $(QEMUOPT) -S

clean:
	make -C boot clean
	make -C lib clean
	make -C user clean
	make -C kernel clean
	rm -f kernel.img
