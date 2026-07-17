CC = gcc
AS = nasm
LD = ld
OBJCOPY = objcopy

# Detect if -Wno-return-mismatch flag works
W_NO_RETURN_MISMATCH := $(shell $(CC) -Werror -Wno-return-mismatch -x c -c /dev/null -o /dev/null 2>/dev/null && echo -Wno-return-mismatch)

CFLAGS = -m32 -ffreestanding -nostdinc -nostdlib -fno-builtin -fno-stack-protector \
         -fno-pic -fno-pie -O2 -Wall -Wextra -Wno-unused-parameter \
         -Wno-implicit-int -Wno-old-style-definition -Wno-return-type \
         -Wno-implicit-function-declaration -Wno-incompatible-pointer-types \
         -Wno-pointer-sign -Wno-sign-compare -Wno-int-conversion \
         $(W_NO_RETURN_MISMATCH) \
         -I. -D__i386__ -DCPM386

ASFLAGS = -f elf32

LDFLAGS = -m elf_i386 -T linker.ld -nostdlib --gc-sections

BDOS_OBJS = bdosmain.o bdosmisc.o bdosrw.o conbdos.o fileio.o dskutil.o iosys.o
CCP_OBJ = ccp.o
BIOS_OBJ = bios.o
BRINGUP_OBJ = cpm_bringup.o

PMODE_OBJS = pmode.o pmode_asm.o
RTC_OBJ = rtc.o
PIT_OBJ = pit.o
OBJS = $(BIOS_OBJ) $(BDOS_OBJS) $(CCP_OBJ) $(BRINGUP_OBJ) $(PMODE_OBJS) $(RTC_OBJ) $(PIT_OBJ) mbentry.o multiboot.o

TARGET = cpm386.elf

MK386 = ./mk386
# 70 KiB image so multi-extent (32 KiB/extent) and >64 KiB
BIG_IMG_SIZE = 71680
RAMDISK_KB = 256

all: $(TARGET) boot.bin os.bin floppy.img

# --- transient .386 programs (load at TPA+0x100) ---
%.bin: %.c user.ld
	$(CC) $(CFLAGS) -c -o $*.o $<
	$(LD) -m elf_i386 -T user.ld -o $*.elf $*.o
	@entry=$$(nm $*.elf | awk '$$NF=="_start"{print $$1}'); \
	  if [ "$$entry" != "00000100" ]; then \
	    echo "ERROR: _start at 0x$$entry, expected 0x100"; \
	    nm $*.elf | head -20; exit 1; \
	  fi
	$(OBJCOPY) -O binary $*.elf $@
	rm -f $*.o $*.elf

$(MK386): mk386.c
	$(CC) -O2 -o $@ $<

# Host helper: punch allocation hole in BIG.386 dirent on a raw CP/M image
PATCH_HOLE = ./patch_hole
$(PATCH_HOLE): patch_hole.c
	$(CC) -O2 -o $@ $<

hello.386: hello.bin $(MK386)
	$(MK386) hello.bin hello.386 0x100

lrbc.386: lrbc.bin $(MK386)
	$(MK386) lrbc.bin lrbc.386 0x100

iotest.386: iotest.bin $(MK386)
	$(MK386) iotest.bin iotest.386 0x100

tod.386: tod.bin $(MK386)
	$(MK386) tod.bin tod.386 0x100

hd.386: hd.bin $(MK386)
	$(MK386) hd.bin hd.386 0x100

od.386: od.bin $(MK386)
	$(MK386) od.bin od.386 0x100

ls.386: ls.bin $(MK386)
	$(MK386) ls.bin ls.386 0x100

ver.386: ver.bin $(MK386)
	$(MK386) ver.bin ver.386 0x100

reboot.386: reboot.bin $(MK386)
	$(MK386) reboot.bin reboot.386 0x100

touch.386: touch.bin $(MK386)
	$(MK386) touch.bin touch.386 0x100

more.386: more.bin $(MK386)
	$(MK386) more.bin more.386 0x100

cls.386: cls.bin $(MK386)
	$(MK386) cls.bin cls.386 0x100

rc.386: rc.bin $(MK386)
	$(MK386) rc.bin rc.386 0x100

tsec.386: tsec.bin $(MK386)
	$(MK386) tsec.bin tsec.386 0x100

trunc.386: trunc.bin $(MK386)
	$(MK386) trunc.bin trunc.386 0x100

rm.386: rm.bin $(MK386)
	$(MK386) rm.bin rm.386 0x100

printenv.386: printenv.bin $(MK386)
	$(MK386) printenv.bin printenv.386 0x100

pause.386: pause.bin $(MK386)
	$(MK386) pause.bin pause.386 0x100

# Console enable helpers (one source, four -DPROG_* builds)
vgaon.bin: conctl.c user.ld
	$(CC) $(CFLAGS) -DPROG_VGAON -c -o vgaon.o conctl.c
	$(LD) -m elf_i386 -T user.ld -o vgaon.elf vgaon.o
	@entry=$$(nm vgaon.elf | awk '$$NF=="_start"{print $$1}'); \
	  if [ "$$entry" != "00000100" ]; then echo "ERROR: vgaon _start"; exit 1; fi
	$(OBJCOPY) -O binary vgaon.elf $@
	rm -f vgaon.o vgaon.elf

vgaoff.bin: conctl.c user.ld
	$(CC) $(CFLAGS) -DPROG_VGAOFF -c -o vgaoff.o conctl.c
	$(LD) -m elf_i386 -T user.ld -o vgaoff.elf vgaoff.o
	@entry=$$(nm vgaoff.elf | awk '$$NF=="_start"{print $$1}'); \
	  if [ "$$entry" != "00000100" ]; then echo "ERROR: vgaoff _start"; exit 1; fi
	$(OBJCOPY) -O binary vgaoff.elf $@
	rm -f vgaoff.o vgaoff.elf

seron.bin: conctl.c user.ld
	$(CC) $(CFLAGS) -DPROG_SERON -c -o seron.o conctl.c
	$(LD) -m elf_i386 -T user.ld -o seron.elf seron.o
	@entry=$$(nm seron.elf | awk '$$NF=="_start"{print $$1}'); \
	  if [ "$$entry" != "00000100" ]; then echo "ERROR: seron _start"; exit 1; fi
	$(OBJCOPY) -O binary seron.elf $@
	rm -f seron.o seron.elf

seroff.bin: conctl.c user.ld
	$(CC) $(CFLAGS) -DPROG_SEROFF -c -o seroff.o conctl.c
	$(LD) -m elf_i386 -T user.ld -o seroff.elf seroff.o
	@entry=$$(nm seroff.elf | awk '$$NF=="_start"{print $$1}'); \
	  if [ "$$entry" != "00000100" ]; then echo "ERROR: seroff _start"; exit 1; fi
	$(OBJCOPY) -O binary seroff.elf $@
	rm -f seroff.o seroff.elf

vgaon.386: vgaon.bin $(MK386)
	$(MK386) vgaon.bin vgaon.386 0x100

vgaoff.386: vgaoff.bin $(MK386)
	$(MK386) vgaoff.bin vgaoff.386 0x100

seron.386: seron.bin $(MK386)
	$(MK386) seron.bin seron.386 0x100

seroff.386: seroff.bin $(MK386)
	$(MK386) seroff.bin seroff.386 0x100

fparse.386: fparse.bin $(MK386)
	$(MK386) fparse.bin fparse.386 0x100

illegal.386: illegal.bin $(MK386)
	$(MK386) illegal.bin illegal.386 0x100

dumpfcb.386: dumpfcb.bin $(MK386)
	$(MK386) dumpfcb.bin dumpfcb.386 0x100

dumpdir.386: dumpdir.bin $(MK386)
	$(MK386) dumpdir.bin dumpdir.386 0x100

mem.386: mem.bin $(MK386)
	$(MK386) mem.bin mem.386 0x100

aclockvt.386: aclockvt.bin $(MK386)
	$(MK386) aclockvt.bin aclockvt.386 0x100

aclockdv.386: aclockdv.bin $(MK386)
	$(MK386) aclockdv.bin aclockdv.386 0x100

vgatext.386: vgatext.bin $(MK386)
	$(MK386) vgatext.bin vgatext.386 0x100

ticks.386: ticks.bin $(MK386)
	$(MK386) ticks.bin ticks.386 0x100

ed.386: ed.bin $(MK386)
	$(MK386) ed.bin ed.386 0x100

# Pad stub to BIG_IMG_SIZE with 0x90 so the .386 spans multiple extents and >64K
big.bin: big.c user.ld
	$(CC) $(CFLAGS) -c -o big.o big.c
	$(LD) -m elf_i386 -T user.ld -o big.elf big.o
	@entry=$$(nm big.elf | awk '$$NF=="_start"{print $$1}'); \
	  if [ "$$entry" != "00000100" ]; then echo "ERROR: big _start"; exit 1; fi
	$(OBJCOPY) -O binary big.elf big_stub.bin
	@stub=$$(wc -c < big_stub.bin); \
	  if [ "$$stub" -ge $(BIG_IMG_SIZE) ]; then \
	    echo "ERROR: big stub $$stub >= $(BIG_IMG_SIZE)"; exit 1; fi; \
	  pad=$$(( $(BIG_IMG_SIZE) - stub )); \
	  cp big_stub.bin big.bin; \
	  dd if=/dev/zero bs=1 count=$$pad status=none 2>/dev/null | \
	    tr '\0' '\220' >> big.bin
	rm -f big.o big.elf big_stub.bin

big.386: big.bin $(MK386)
	$(MK386) big.bin big.386 0x100

# RAM disk: programs + accurate DOS-PLUS LRBC (cpmtools)
ramdisk.bin: hello.386 lrbc.386 iotest.386 big.386 tod.386 hd.386 od.386 ls.386 ver.386 reboot.386 touch.386 more.386 cls.386 rc.386 tsec.386 trunc.386 rm.386 printenv.386 pause.386 vgaon.386 vgaoff.386 seron.386 seroff.386 fparse.386 illegal.386 dumpfcb.386 dumpdir.386 mem.386 aclockvt.386 aclockdv.386 vgatext.386 ticks.386 ed.386 $(PATCH_HOLE)
	mkdir -p /tmp/cpmd
	printf 'This is README.TXT from the CP/M-386 RAM disk.\r\n\x1a' > /tmp/cpmd/README.TXT
	# Sample environment file for PRINTENV (upstream-style VAR=value)
	printf '; Sample ENV.DAT for CP/M-386\r\nHELLO=World\r\n\x1a' > /tmp/cpmd/ENV.DAT
	# SUBMIT demo (plain CR/LF lines; ; comments; CP/M-68K CCP style)
	printf '; DEMO.SUB - exercise SUBMIT on CP/M-386\r\nVER\r\nPRINTENV\r\nDIR\r\n\x1a' > /tmp/cpmd/DEMO.SUB
	# Cold-boot profile: auto-run once at power-on if present (see try_cold_profile)
	printf 'VER\r\n' > /tmp/cpmd/PROFILE.SUB
	cp hello.386 /tmp/cpmd/HELLO.386
	cp lrbc.386 /tmp/cpmd/LRBC.386
	cp iotest.386 /tmp/cpmd/IOTEST.386
	cp big.386 /tmp/cpmd/BIG.386
	cp tod.386 /tmp/cpmd/TOD.386
	cp hd.386 /tmp/cpmd/HD.386
	cp od.386 /tmp/cpmd/OD.386
	cp ls.386 /tmp/cpmd/LS.386
	cp ver.386 /tmp/cpmd/VER.386
	cp reboot.386 /tmp/cpmd/REBOOT.386
	cp touch.386 /tmp/cpmd/TOUCH.386
	cp more.386 /tmp/cpmd/MORE.386
	cp cls.386 /tmp/cpmd/CLS.386
	cp rc.386 /tmp/cpmd/RC.386
	cp tsec.386 /tmp/cpmd/TSEC.386
	cp trunc.386 /tmp/cpmd/TRUNC.386
	cp rm.386 /tmp/cpmd/RM.386
	cp printenv.386 /tmp/cpmd/PRINTENV.386
	cp pause.386 /tmp/cpmd/PAUSE.386
	cp vgaon.386 /tmp/cpmd/VGAON.386
	cp vgaoff.386 /tmp/cpmd/VGAOFF.386
	cp seron.386 /tmp/cpmd/SERON.386
	cp seroff.386 /tmp/cpmd/SEROFF.386
	cp fparse.386 /tmp/cpmd/FPARSE.386
	cp illegal.386 /tmp/cpmd/ILLEGAL.386
	cp dumpfcb.386 /tmp/cpmd/DUMPFCB.386
	cp dumpdir.386 /tmp/cpmd/DUMPDIR.386
	cp mem.386 /tmp/cpmd/MEM.386
	cp aclockvt.386 /tmp/cpmd/ACLOCKVT.386
	cp aclockdv.386 /tmp/cpmd/ACLOCKDV.386
	cp vgatext.386 /tmp/cpmd/VGATEXT.386
	cp ticks.386 /tmp/cpmd/TICKS.386
	cp ed.386 /tmp/cpmd/ED.386
	mkfs.cpm -f 4mb-hd /tmp/ramdisk.tmp
	cpmcp -f 4mb-hd /tmp/ramdisk.tmp \
	  /tmp/cpmd/README.TXT /tmp/cpmd/ENV.DAT /tmp/cpmd/DEMO.SUB \
	  /tmp/cpmd/PROFILE.SUB \
	  /tmp/cpmd/HELLO.386 /tmp/cpmd/LRBC.386 \
	  /tmp/cpmd/IOTEST.386 /tmp/cpmd/BIG.386 /tmp/cpmd/TOD.386 \
	  /tmp/cpmd/HD.386 /tmp/cpmd/OD.386 /tmp/cpmd/LS.386 \
	  /tmp/cpmd/VER.386 /tmp/cpmd/REBOOT.386 /tmp/cpmd/TOUCH.386 \
	  /tmp/cpmd/MORE.386 /tmp/cpmd/CLS.386 /tmp/cpmd/RC.386 \
	  /tmp/cpmd/TSEC.386 /tmp/cpmd/TRUNC.386 /tmp/cpmd/RM.386 \
	  /tmp/cpmd/PRINTENV.386 /tmp/cpmd/PAUSE.386 \
	  /tmp/cpmd/VGAON.386 /tmp/cpmd/VGAOFF.386 \
	  /tmp/cpmd/SERON.386 /tmp/cpmd/SEROFF.386 /tmp/cpmd/FPARSE.386 \
	  /tmp/cpmd/ILLEGAL.386 /tmp/cpmd/DUMPFCB.386 /tmp/cpmd/DUMPDIR.386 \
	  /tmp/cpmd/MEM.386 /tmp/cpmd/ACLOCKVT.386 /tmp/cpmd/ACLOCKDV.386 \
	  /tmp/cpmd/VGATEXT.386 /tmp/cpmd/TICKS.386 /tmp/cpmd/ED.386 0:
	# Patch BIG.386 dirent: zero a middle allocation block that only covers
	# 0x90 padding for sparse hole; streaming loader zero-fills it.
	$(PATCH_HOLE) /tmp/ramdisk.tmp BIG.386 || true
	# Fixed-size image: zero-fill then overlay cpmtools filesystem (dd count
	# alone stops at EOF and would shrink as the FS grows).
	dd if=/dev/zero of=ramdisk.bin bs=1024 count=$(RAMDISK_KB) status=none 2>/dev/null
	dd if=/tmp/ramdisk.tmp of=ramdisk.bin conv=notrunc status=none 2>/dev/null
	rm -rf /tmp/ramdisk.tmp /tmp/cpmd

cpm_bringup.o: cpm_bringup.c ramdisk.bin cpm_bringup.h
	$(CC) $(CFLAGS) -c -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

bios.o: bios.c
	$(CC) $(CFLAGS) -c -o $@ $<

rtc.o: rtc.c rtc.h
	$(CC) $(CFLAGS) -c -o $@ $<

pit.o: pit.c pit.h
	$(CC) $(CFLAGS) -c -o $@ $<

pmode.o: pmode.c pmode.h
	$(CC) $(CFLAGS) -c -o $@ $<

pmode_asm.o: pmode.S
	$(AS) $(ASFLAGS) -I . -o $@ $<

mbentry.o: mbentry.S multiboot.h
	$(AS) $(ASFLAGS) -I . -o $@ $<

bss.inc: $(TARGET)
	nm $(TARGET) | awk '/__bss_start/ { print "bss_start equ 0x" $$1 }' > bss.inc
	nm $(TARGET) | awk '/__bss_end/ { print "bss_end equ 0x" $$1 }' >> bss.inc
	nm $(TARGET) | awk '/__kernel_end/ { print "kernel_end equ 0x" $$1 }' >> bss.inc
	nm $(TARGET) | awk '/ _start$$/ { print "kernel_entry equ 0x" $$1 }' >> bss.inc
	# Floppy boot loads this many 512-byte sectors of os.bin at 0x10000.
	# Must cover kernel_end (code+data+bss hole+ramdisk); +2 for headroom.
	ke=$$(nm $(TARGET) | awk '/__kernel_end/ { print $$1 }'); \
	  bytes=$$((0x$$ke - 0x10000)); \
	  sec=$$(( (bytes + 511) / 512 + 2 )); \
	  echo "SECTORS_TO_LOAD equ $$sec" >> bss.inc

boot.bin: boot.S bss.inc
	$(AS) -f bin -I . -o $@ $<

os.bin: $(TARGET)
	$(OBJCOPY) -O binary $(TARGET) $@

$(TARGET): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

floppy.img: boot.bin os.bin
	cat boot.bin os.bin > payload.bin
	# Full 1.44M image
	dd if=/dev/zero of=$@ bs=512 count=2880 status=none 2>/dev/null
	dd if=payload.bin of=$@ conv=notrunc status=none 2>/dev/null

clean:
	rm -f *.o $(TARGET) *.img *.log cpm386.bin test_bdos os.bin boot.bin \
	      payload.bin bss.inc ramdisk.bin \
	      hello.bin hello.386 lrbc.bin lrbc.386 iotest.bin iotest.386 \
	      big.bin big.386 tod.bin tod.386 hd.bin hd.386 od.bin od.386 \
	      ls.bin ls.386 ver.bin ver.386 reboot.bin reboot.386 \
	      touch.bin touch.386 more.bin more.386 cls.bin cls.386 \
	      rc.bin rc.386 tsec.bin tsec.386 trunc.bin trunc.386 \
	      rm.bin rm.386 printenv.bin printenv.386 pause.bin pause.386 \
	      vgaon.bin vgaon.386 vgaoff.bin vgaoff.386 \
	      seron.bin seron.386 seroff.bin seroff.386 \
	      fparse.bin fparse.386 illegal.bin illegal.386 \
	      dumpfcb.bin dumpfcb.386 dumpdir.bin dumpdir.386 \
	      mem.bin mem.386 aclockvt.bin aclockvt.386 \
	      aclockdv.bin aclockdv.386 vgatext.bin vgatext.386 \
	      ticks.bin ticks.386 ed.bin ed.386 \
	      $(MK386) $(PATCH_HOLE)


run: floppy.img
	timeout 5s qemu-system-i386 -nographic -serial stdio -monitor none \
	  -drive if=floppy,format=raw,file=floppy.img -boot a || true

test_bdos: test_bdos.c bdosmain.o bdosmisc.o bdosrw.o conbdos.o fileio.o dskutil.o iosys.o ccp.o cpm_bringup.o
	$(CC) -m32 -O2 -I. -o $@ $^

test: test_bdos
	./test_bdos

scc:
	"$${MAKE:-$(MAKE)}" scc-real
	"$${MAKE:-$(MAKE)}" scc-real

scc-real: README.md
	"$${MAKE:-$(MAKE)}" clean
	awk '/<!-- scc-start -->/ { \
		print; system("scc \
			--exclude-file LICENSE,README.awk,log.pvs \
			--exclude-file log.pvs,compile_commands.json \
			--exclude-file REUSE.toml \
			--exclude-dir LICENSES,.git,pvsreport,bindist \
			--no-cocomo -u --no-size -s lines -f html-table; \
			printf \"\n%s\n\" \"<!-- scc-end -->\""); \
			skip=1; next } \
		skip && /<!-- scc-end -->/ { skip=0; next } \
		!skip' README.md > README.awk && \
	mv -f README.awk README.md && \
	expand README.md > README.out && \
	mv -f README.out README.md

.NOTPARALLEL:
.PHONY: all clean run test scc scc-real
