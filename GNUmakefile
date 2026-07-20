################################################################################
# CP/M-386 - Makefile
# Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
# SPDX-License-Identifier: MIT
# scspell-id: b1bb2880-826f-11f1-a5c2-80ee73e9b8e7
################################################################################

################################################################################

ifndef DEBUG
 DEBUGFLAGS=-DNDEBUG
 LMAP=
 NASMDEBUG=
 OPTFLAGS=-O2
else
 DEBUGFLAGS=-DDEBUG
 LMAP=--print-map --cref
 NASMDEBUG=-g
 OPTFLAGS=-Og -ggdb -fdata-sections -ffunction-sections
endif

################################################################################

PRINTF:=$(shell \
	command -v gprintf 2> /dev/null || \
	command -v printf 2> /dev/null || \
	printf '%s' "printf")

################################################################################

CC:=$(shell \
	command -v gcc 2> /dev/null || \
	command -v clang 2> /dev/null || \
	$(PRINTF) '%s' "cc")

################################################################################

AS:=$(shell \
	command -v nasm 2> /dev/null || \
	command -v nasm-segelf 2> /dev/null || \
	$(PRINTF) '%s' "nasm")

################################################################################

NM:=$(shell \
	command -v gnm 2> /dev/null || \
	command -v nm 2> /dev/null || \
	command -v llvm-nm 2> /dev/null || \
	$(PRINTF) '%s' "nm")

################################################################################

LD:=$(shell \
	command -v gld 2> /dev/null || \
	command -v ld.bfd 2> /dev/null || \
	command -v ld.gold 2> /dev/null || \
	command -v ld.lld 2> /dev/null || \
	command -v ld > /dev/null || \
	$(PRINTF) '%s' "ld")

################################################################################

OBJCOPY:=$(shell \
	command -v gobjcopy 2> /dev/null || \
	command -v objcopy 2> /dev/null || \
	command -v llvm-objcopy 2> /dev/null || \
	$(PRINTF) '%s' "objcopy")

################################################################################

STRIP:=$(shell \
	command -v gstrip 2> /dev/null || \
	command -v strip 2> /dev/null || \
	command -v llvm-strip 2> /dev/null || \
	$(PRINTF) '%s' "strip")

################################################################################

SIZE:=$(shell \
	command -v size 2> /dev/null || \
	command -v llvm-size 2> /dev/null || \
	$(PRINTF) '%s' "size")

################################################################################

AWK:=$(shell \
	command -v gawk 2> /dev/null || \
	command -v mawk 2> /dev/null || \
	command -v nawk 2> /dev/null || \
	command -v oawk 2> /dev/null || \
	command -v awk 2> /dev/null || \
	$(PRINTF) '%s' "awk")

################################################################################

DD:=$(shell \
	command -v gdd 2> /dev/null || \
	command -v dd 2> /dev/null || \
	$(PRINTF) '%s' "dd")

################################################################################

OD:=$(shell \
	command -v god 2> /dev/null || \
	command -v od 2> /dev/null || \
	$(PRINTF) '%s' "od")

################################################################################

EXPAND:=$(shell \
	command -v gexpand 2> /dev/null || \
	command -v expand 2> /dev/null || \
	$(PRINTF) '%s' "expand")

################################################################################

GREP:=$(shell \
	command -v ggrep 2> /dev/null || \
	command -v grep 2> /dev/null || \
	$(PRINTF) '%s' "grep")

################################################################################

W_NO_RETURN_MISMATCH:=$(shell \
	$(CC) -Werror -Wno-return-mismatch \
	-x c -c /dev/null -o /dev/null 2> /dev/null && \
	$(PRINTF) '%s' "-Wno-return-mismatch")

################################################################################

W_NO_DEPRECATED_NON_PROTOTYPE:=$(shell \
	$(CC) -Werror -Wno-deprecated-non-prototype \
	-x c -c /dev/null -o /dev/null 2> /dev/null && \
	$(PRINTF) '%s' "-Wno-deprecated-non-prototype")

################################################################################

WNO_UNUSED_COMMAND_LINE_ARGUMENT:=$(shell \
	$(CC) -Werror -Wno-unused-command-line-argument \
	-x c -c /dev/null -o /dev/null 2> /dev/null && \
	$(PRINTF) '%s' "-Wno-unused-command-line-argument")

################################################################################

FNO_STACK_CLASH_PROTECTION:=$(shell \
	$(CC) -Werror -fno-stack-clash-protection \
	-x c c /dev/null -o /dev/null 2> /dev/null && \
	$(PRINTF) '%s' "-fno-stack-clash-protection")

################################################################################

OS=$(shell uname -s 2> /dev/null)

################################################################################

ELF_I386=elf_i386

################################################################################

ifneq "$(findstring SunOS,$(OS))" ""
 ELF_I386=elf_i386_sol2
endif

################################################################################

ifneq "$(findstring Haiku,$(OS))" ""
 ELF_I386=elf_i386_haiku
endif

################################################################################

CFLAGS = \
	$(DEBUGFLAGS) \
	$(FNO_STACK_CLASH_PROTECTION) \
	$(OPTFLAGS) \
	$(W_NO_DEPRECATED_NON_PROTOTYPE) \
	$(W_NO_RETURN_MISMATCH) \
	$(WNO_UNUSED_COMMAND_LINE_ARGUMENT) \
	-D__cpm386 \
	-D__cpm386__ \
	-Dcpm386 \
	-D__CPM386 \
	-D__CPM386__ \
	-DCPM386 \
	-D__i386 \
	-D__i386__ \
	-D__I386 \
	-D__I386__ \
	-DRAMDISK_KB=$(RAMDISK_KB) \
	-ffreestanding \
	-fmerge-all-constants \
	-fno-asynchronous-unwind-tables \
	-fno-builtin \
	-fno-pic \
	-fno-pie \
	-fno-plt \
	-fno-stack-protector \
	-fno-tree-vectorize \
	-fno-unwind-tables \
	-fomit-frame-pointer \
	-I. \
	-m32 \
	-march=i386 \
	-mtune=i686 \
	-nostdinc \
	-nostdlib \
	-U_FORTIFY_SOURCE \
	-Wall \
	-Wcast-qual \
	-Wdouble-promotion \
	-Wextra \
	-Wformat-security \
	-Wno-implicit-function-declaration \
	-Wno-implicit-int \
	-Wno-incompatible-pointer-types \
	-Wno-int-conversion \
	-Wno-old-style-definition \
	-Wno-pointer-sign \
	-Wno-return-type \
	-Wno-sign-compare \
	-Wno-unused-parameter \
	-Wshadow

################################################################################

ASFLAGS = -f elf32

################################################################################

LDFLAGS = -m $(ELF_I386) -no-pie -T linker.ld -nostdlib \
	--gc-sections --print-gc-sections

################################################################################

BDOS_OBJS = bdosmain.o bdosmisc.o bdosrw.o conbdos.o fileio.o dskutil.o iosys.o
CCP_OBJ = ccp.o
BIOS_OBJ = bios.o
BRINGUP_OBJ = bringup.o
PMODE_OBJS = pmode.o pmodeasm.o
RTC_OBJ = rtc.o
PIT_OBJ = pit.o
OBJS = $(BIOS_OBJ) $(BDOS_OBJS) $(CCP_OBJ) $(BRINGUP_OBJ) $(PMODE_OBJS) \
	$(RTC_OBJ) $(PIT_OBJ) mbentry.o mltiboot.o

################################################################################

TARGET = cpm386.elf

################################################################################

MK386 = mk386

################################################################################

# 70 KiB image: multi-extent and >64 KiB
BIG_IMG_SIZE = 71680

################################################################################

# Truncate ramdisk image to 256K - unsafe but temporary for bringup
RAMDISK_KB = 256

################################################################################

.PHONY: all

all: $(TARGET) boot.bin os.bin floppy.img

################################################################################

.PHONY: strip

strip: $(TARGET) floppy.img
	$(STRIP) -R '.shstrtab' -R '.strtab' -R '.symtab' "./$(TARGET)" || :

################################################################################

%.bin: %.c user.ld
	$(CC) $(CFLAGS) -c -o ./$*.o ./$<
	$(LD) $(LMAP) -m $(ELF_I386) -no-pie -T ./user.ld \
		-o ./$*.elf ./$*.o
	@entry=$$($(NM) ./$*.elf | $(AWK) '$$NF=="_start"{print $$1}'); \
	  if [ "$$entry" != "00000100" ]; then \
	    $(PRINTF) '%s\n' "ERROR: _start at 0x$$entry, expected 0x100"; \
	    $(NM) ./$*.elf | head -20; \
	    exit 1; fi
	$(OBJCOPY) -O binary ./$*.elf ./$@
	rm -f ./$*.o ./$*.elf

################################################################################

$(MK386): mk386.c
	$(CC) $(OPTFLAGS) -o ./$@ ./$<

################################################################################

SHOLE = shole
$(SHOLE): shole.c
	$(CC) $(OPTFLAGS) -o ./$@ ./$<

################################################################################

hello.386: hello.bin $(MK386)
	./$(MK386) ./hello.bin ./hello.386 0x100
	rm -f ./hello.bin

################################################################################

test211.386: test211.bin $(MK386)
	./$(MK386) ./test211.bin ./test211.386 0x100
	rm -f ./test211.bin

################################################################################

stat.386: stat.bin $(MK386)
	./$(MK386) ./stat.bin ./stat.386 0x100
	rm -f ./stat.bin

################################################################################

lrbc.386: lrbc.bin $(MK386)
	./$(MK386) ./lrbc.bin ./lrbc.386 0x100
	rm -f ./lrbc.bin

################################################################################

iotest.386: iotest.bin $(MK386)
	./$(MK386) ./iotest.bin ./iotest.386 0x100
	rm -f ./iotest.bin

################################################################################

delay.386: delay.bin $(MK386)
	./$(MK386) ./delay.bin ./delay.386 0x100
	rm -f ./delay.bin

################################################################################

getsn.386: getsn.bin $(MK386)
	./$(MK386) ./getsn.bin ./getsn.386 0x100
	rm -f ./getsn.bin

################################################################################

sync.386: sync.bin $(MK386)
	./$(MK386) ./sync.bin ./sync.386 0x100
	rm -f ./sync.bin

################################################################################

tod.386: tod.bin $(MK386)
	./$(MK386) ./tod.bin ./tod.386 0x100
	rm -f ./tod.bin

################################################################################

hd.386: hd.bin $(MK386)
	./$(MK386) ./hd.bin ./hd.386 0x100
	rm -f ./hd.bin

################################################################################

od.386: od.bin $(MK386)
	./$(MK386) ./od.bin ./od.386 0x100
	rm -f ./od.bin

################################################################################

ls.386: ls.bin $(MK386)
	./$(MK386) ./ls.bin ./ls.386 0x100
	rm -f ./ls.bin

################################################################################

ver.386: ver.bin $(MK386)
	./$(MK386) ./ver.bin ./ver.386 0x100
	rm -f ./ver.bin

################################################################################

reboot.386: reboot.bin $(MK386)
	./$(MK386) ./reboot.bin ./reboot.386 0x100
	rm -f ./reboot.bin

################################################################################

touch.386: touch.bin $(MK386)
	./$(MK386) ./touch.bin ./touch.386 0x100
	rm -f ./touch.bin

################################################################################

more.386: more.bin $(MK386)
	./$(MK386) ./more.bin ./more.386 0x100
	rm -f ./more.bin

################################################################################

cls.386: cls.bin $(MK386)
	./$(MK386) cls.bin cls.386 0x100
	rm -f ./cls.bin

################################################################################

rc.386: rc.bin $(MK386)
	./$(MK386) ./rc.bin ./rc.386 0x100
	rm -f ./rc.bin

################################################################################

tsec.386: tsec.bin $(MK386)
	./$(MK386) ./tsec.bin ./tsec.386 0x100
	rm -f ./tsec.bin

################################################################################

trunc.386: trunc.bin $(MK386)
	./$(MK386) ./trunc.bin ./trunc.386 0x100
	rm -f ./trunc.bin

################################################################################

rm.386: rm.bin $(MK386)
	./$(MK386) ./rm.bin ./rm.386 0x100
	rm -f ./rm.bin

################################################################################

printenv.386: printenv.bin $(MK386)
	./$(MK386) ./printenv.bin ./printenv.386 0x100
	rm -f ./printenv.bin

################################################################################

pause.386: pause.bin $(MK386)
	./$(MK386) ./pause.bin ./pause.386 0x100
	rm -f ./pause.bin

################################################################################

vgaon.bin: conctl.c user.ld
	$(CC) $(CFLAGS) -DPROG_VGAON -c -o ./vgaon.o ./conctl.c
	$(LD) $(LMAP) -m $(ELF_I386) -no-pie -T ./user.ld \
		-o ./vgaon.elf ./vgaon.o
	@entry=$$($(NM) ./vgaon.elf | $(AWK) '$$NF=="_start"{print $$1}'); \
	  if [ "$$entry" != "00000100" ]; then \
	  $(PRINTF) '%s\n' "ERROR: vgaon _start"; \
	  exit 1; fi
	$(OBJCOPY) -O binary ./vgaon.elf ./$@
	rm -f ./vgaon.o ./vgaon.elf

################################################################################

vgaoff.bin: conctl.c user.ld
	$(CC) $(CFLAGS) -DPROG_VGAOFF -c -o ./vgaoff.o ./conctl.c
	$(LD) $(LMAP) -m $(ELF_I386) -no-pie -T ./user.ld \
		-o ./vgaoff.elf ./vgaoff.o
	@entry=$$($(NM) ./vgaoff.elf | $(AWK) '$$NF=="_start"{print $$1}'); \
	  if [ "$$entry" != "00000100" ]; then \
	  $(PRINTF) '%s\n' "ERROR: vgaoff _start"; \
	  exit 1; fi
	$(OBJCOPY) -O binary ./vgaoff.elf ./$@
	rm -f ./vgaoff.o ./vgaoff.elf

################################################################################

seron.bin: conctl.c user.ld
	$(CC) $(CFLAGS) -DPROG_SERON -c -o ./seron.o ./conctl.c
	$(LD) $(LMAP) -m $(ELF_I386) -no-pie -T ./user.ld \
		-o ./seron.elf ./seron.o
	@entry=$$($(NM) ./seron.elf | $(AWK) '$$NF=="_start"{print $$1}'); \
	  if [ "$$entry" != "00000100" ]; then \
	  $(PRINTF) '%s\n' "ERROR: seron _start"; \
	  exit 1; fi
	$(OBJCOPY) -O binary ./seron.elf ./$@
	rm -f ./seron.o ./seron.elf

################################################################################

seroff.bin: conctl.c user.ld
	$(CC) $(CFLAGS) -DPROG_SEROFF -c -o ./seroff.o ./conctl.c
	$(LD) $(LMAP) -m $(ELF_I386) -no-pie -T ./user.ld \
		-o ./seroff.elf ./seroff.o
	@entry=$$($(NM) ./seroff.elf | $(AWK) '$$NF=="_start"{print $$1}'); \
	  if [ "$$entry" != "00000100" ]; then \
	  $(PRINTF) '%s\n' "ERROR: seroff _start"; \
	  exit 1; fi
	$(OBJCOPY) -O binary ./seroff.elf ./$@
	rm -f ./seroff.o ./seroff.elf

################################################################################

vgaon.386: vgaon.bin $(MK386)
	./$(MK386) ./vgaon.bin ./vgaon.386 0x100
	rm -f ./vgaon.bin

################################################################################

vgaoff.386: vgaoff.bin $(MK386)
	./$(MK386) ./vgaoff.bin ./vgaoff.386 0x100
	rm -f ./vgaoff.bin

################################################################################

seron.386: seron.bin $(MK386)
	./$(MK386) ./seron.bin ./seron.386 0x100
	rm -f ./seron.bin

################################################################################

seroff.386: seroff.bin $(MK386)
	./$(MK386) ./seroff.bin ./seroff.386 0x100
	rm -f ./seroff.bin

################################################################################

fparse.386: fparse.bin $(MK386)
	./$(MK386) ./fparse.bin ./fparse.386 0x100
	rm -f ./fparse.bin

################################################################################

illegal.386: illegal.bin $(MK386)
	./$(MK386) ./illegal.bin ./illegal.386 0x100
	rm -f ./illegal.bin

################################################################################

dumpfcb.386: dumpfcb.bin $(MK386)
	./$(MK386) ./dumpfcb.bin ./dumpfcb.386 0x100
	rm -f ./dumpfcb.bin

################################################################################

dumpdir.386: dumpdir.bin $(MK386)
	./$(MK386) ./dumpdir.bin ./dumpdir.386 0x100
	rm -f dumpdir.bin

################################################################################

mem.386: mem.bin $(MK386)
	./$(MK386) ./mem.bin ./mem.386 0x100
	rm -f ./mem.bin

################################################################################

aclockvt.386: aclockvt.bin $(MK386)
	./$(MK386) ./aclockvt.bin ./aclockvt.386 0x100
	rm -f ./aclockvt.bin

################################################################################

aclockdv.386: aclockdv.bin $(MK386)
	./$(MK386) ./aclockdv.bin ./aclockdv.386 0x100
	rm -f ./aclockdv.bin

################################################################################

vgatext.386: vgatext.bin $(MK386)
	./$(MK386) ./vgatext.bin ./vgatext.386 0x100
	rm -f ./vgatext.bin

################################################################################

ticks.386: ticks.bin $(MK386)
	./$(MK386) ./ticks.bin ./ticks.386 0x100
	rm -f ./ticks.bin

################################################################################

big.bin: big.c user.ld
	$(CC) $(CFLAGS) -c -o ./big.o ./big.c
	$(LD) $(LMAP) -m $(ELF_I386) -no-pie -T ./user.ld \
		-o ./big.elf ./big.o
	@entry=$$($(NM) ./big.elf | $(AWK) '$$NF=="_start"{print $$1}'); \
	  if [ "$$entry" != "00000100" ]; then \
	  $(PRINTF) '%s\n' "ERROR: big _start"; \
	  exit 1; fi
	$(OBJCOPY) -O binary ./big.elf ./big_stub.bin
	@stub=$$(wc -c < ./big_stub.bin); \
	  if [ "$$stub" -ge $(BIG_IMG_SIZE) ]; then \
	    $(PRINTF) '%s\n' "ERROR: big stub $$stub >= $(BIG_IMG_SIZE)"; \
	    exit 1; fi; \
	  pad=$$(( $(BIG_IMG_SIZE) - stub )); \
	  cp -f ./big_stub.bin ./big.bin; \
	  $(DD) if="/dev/zero" bs="1" count="$$pad" | tr '\0' '\220' >> ./big.bin
	rm -f ./big.o ./big.elf ./big_stub.bin

################################################################################

big.386: big.bin $(MK386)
	./$(MK386) ./big.bin ./big.386 0x100
	rm -f ./big.bin

################################################################################

ramdisk.bin: hello.386 lrbc.386 iotest.386 big.386 tod.386 hd.386 od.386 \
		ls.386 ver.386 reboot.386 touch.386 more.386 cls.386 rc.386 tsec.386 \
		trunc.386 rm.386 printenv.386 pause.386 vgaon.386 vgaoff.386 \
		seron.386 seroff.386 fparse.386 illegal.386 dumpfcb.386 dumpdir.386 \
		mem.386 aclockvt.386 aclockdv.386 vgatext.386 ticks.386 delay.386 \
		getsn.386 sync.386 test211.386 stat.386 $(SHOLE)
	rm -f /tmp/ramdisk.tmp
	rm -rf /tmp/cpmd
	mkdir -p /tmp/cpmd
	$(PRINTF) 'This is README.TXT from the CP/M-386 RAM disk.\r\n\x1a' \
		> /tmp/cpmd/README.TXT
	$(PRINTF) '; Sample ENV.DAT for CP/M-386\r\nHELLO=World\r\n\x1a' \
		> /tmp/cpmd/ENV.DAT
	$(PRINTF) '; DEMO.SUB - SUBMIT on CP/M-386 testing\r\n; NOTE: No nested SUBMIT (yet!)\r\nSEROFF\r\nSERON\r\nVGAOFF\r\nVGAON\r\nVER\r\nGETSN\r\nMEM\r\nTOD\r\nTSEC\r\nTICKS\r\nTOD\r\nTSEC\r\nTICKS\r\nTOD\r\nTSEC\r\nTICKS\r\nDELAY\r\nSTAT DSK:\r\nSTAT STAT.386 SIZE\r\nPRINTENV\r\nTEST211\r\nLS -A\r\nDIR\r\nLS -L BIG.*\r\nBIG\r\nLRBC BIG.386\r\nTRUNC\r\nIOTEST\r\nSYNC\r\nFPARSE\r\nILLEGAL\r\nLRBC README.TXT\r\nTOUCH NEW.DAT\r\nERA NEW.DAT\r\nDUMPFCB DEMO.SUB\r\nDUMPDIR DEMO.*\r\nHD CLS.386\r\nOD CLS.386\r\nERA TRUNC.DAT\r\nRC 1\r\nREN IOWORK.D4T=IOWORK.DAT\r\nRM IOWORK.D4T\r\nRC\r\nHELLO\r\n; Run again from program still existing in TPA\r\nGO\r\n; End of DEMO.SUB\r\n\x1a' \
		> /tmp/cpmd/DEMO.SUB
	$(PRINTF) 'VER\r\n' > /tmp/cpmd/PROFILE.SUB
	cp -f ./aclockdv.386 /tmp/cpmd/ACLOCKDV.386
	cp -f ./aclockvt.386 /tmp/cpmd/ACLOCKVT.386
	cp -f ./big.386 /tmp/cpmd/BIG.386
	cp -f ./cls.386 /tmp/cpmd/CLS.386
	cp -f ./delay.386 /tmp/cpmd/DELAY.386
	cp -f ./dumpdir.386 /tmp/cpmd/DUMPDIR.386
	cp -f ./dumpfcb.386 /tmp/cpmd/DUMPFCB.386
	cp -f ./fparse.386 /tmp/cpmd/FPARSE.386
	cp -f ./getsn.386 /tmp/cpmd/GETSN.386
	cp -f ./hd.386 /tmp/cpmd/HD.386
	cp -f ./hello.386 /tmp/cpmd/HELLO.386
	cp -f ./illegal.386 /tmp/cpmd/ILLEGAL.386
	cp -f ./iotest.386 /tmp/cpmd/IOTEST.386
	cp -f ./lrbc.386 /tmp/cpmd/LRBC.386
	cp -f ./ls.386 /tmp/cpmd/LS.386
	cp -f ./mem.386 /tmp/cpmd/MEM.386
	cp -f ./more.386 /tmp/cpmd/MORE.386
	cp -f ./od.386 /tmp/cpmd/OD.386
	cp -f ./pause.386 /tmp/cpmd/PAUSE.386
	cp -f ./printenv.386 /tmp/cpmd/PRINTENV.386
	cp -f ./rc.386 /tmp/cpmd/RC.386
	cp -f ./reboot.386 /tmp/cpmd/REBOOT.386
	cp -f ./rm.386 /tmp/cpmd/RM.386
	cp -f ./seroff.386 /tmp/cpmd/SEROFF.386
	cp -f ./seron.386 /tmp/cpmd/SERON.386
	cp -f ./stat.386 /tmp/cpmd/STAT.386
	cp -f ./sync.386 /tmp/cpmd/SYNC.386
	cp -f ./test211.386 /tmp/cpmd/TEST211.386
	cp -f ./ticks.386 /tmp/cpmd/TICKS.386
	cp -f ./tod.386 /tmp/cpmd/TOD.386
	cp -f ./touch.386 /tmp/cpmd/TOUCH.386
	cp -f ./trunc.386 /tmp/cpmd/TRUNC.386
	cp -f ./tsec.386 /tmp/cpmd/TSEC.386
	cp -f ./ver.386 /tmp/cpmd/VER.386
	cp -f ./vgaoff.386 /tmp/cpmd/VGAOFF.386
	cp -f ./vgaon.386 /tmp/cpmd/VGAON.386
	cp -f ./vgatext.386 /tmp/cpmd/VGATEXT.386
	mkfs.cpm -f 4mb-hd /tmp/ramdisk.tmp
	cpmcp -f 4mb-hd /tmp/ramdisk.tmp \
	  /tmp/cpmd/ACLOCKDV.386 \
	  /tmp/cpmd/ACLOCKVT.386 \
	  /tmp/cpmd/BIG.386 \
	  /tmp/cpmd/CLS.386 \
	  /tmp/cpmd/DELAY.386 \
	  /tmp/cpmd/DEMO.SUB \
	  /tmp/cpmd/DUMPDIR.386 \
	  /tmp/cpmd/DUMPFCB.386 \
	  /tmp/cpmd/ENV.DAT \
	  /tmp/cpmd/FPARSE.386 \
	  /tmp/cpmd/GETSN.386 \
	  /tmp/cpmd/HD.386 \
	  /tmp/cpmd/HELLO.386 \
	  /tmp/cpmd/ILLEGAL.386 \
	  /tmp/cpmd/IOTEST.386 \
	  /tmp/cpmd/LRBC.386 \
	  /tmp/cpmd/LS.386 \
	  /tmp/cpmd/MEM.386 \
	  /tmp/cpmd/MORE.386 \
	  /tmp/cpmd/OD.386 \
	  /tmp/cpmd/PAUSE.386 \
	  /tmp/cpmd/PRINTENV.386 \
	  /tmp/cpmd/PROFILE.SUB \
	  /tmp/cpmd/RC.386 \
	  /tmp/cpmd/README.TXT \
	  /tmp/cpmd/REBOOT.386 \
	  /tmp/cpmd/RM.386 \
	  /tmp/cpmd/SEROFF.386 \
	  /tmp/cpmd/SERON.386 \
	  /tmp/cpmd/STAT.386 \
	  /tmp/cpmd/SYNC.386 \
	  /tmp/cpmd/TEST211.386 \
	  /tmp/cpmd/TICKS.386 \
	  /tmp/cpmd/TOD.386 \
	  /tmp/cpmd/TOUCH.386 \
	  /tmp/cpmd/TRUNC.386 \
	  /tmp/cpmd/TSEC.386 \
	  /tmp/cpmd/VER.386 \
	  /tmp/cpmd/VGAOFF.386 \
	  /tmp/cpmd/VGAON.386 \
	  /tmp/cpmd/VGATEXT.386 \
	  0:
	./$(SHOLE) /tmp/ramdisk.tmp BIG.386
	@RDS=$$($(PRINTF) '%d' "$$($(OD) -A x -t x2 /tmp/ramdisk.tmp | \
		$(GREP) -v '^*$$' | tail -2 | head -1 | \
		$(AWK) '{ print "0x"a$$1 }')"); \
		RDS=$$(( (RDS / 1024) + 4 )); \
		$(PRINTF) '*** ramdisk.tmp usage: ~%s KB\n' "$$(( RDS - 4))"; \
		test "$$RDS" -lt "$(RAMDISK_KB)" || { \
		$(PRINTF) '%s\n' \
			"*** ERROR: ramdisk too large for $(RAMDISK_KB) space reserved!"; \
			exit 2; }
	$(DD) if="/dev/zero" of="./ramdisk.bin" bs="1024" count="$(RAMDISK_KB)"
	$(DD) if="/tmp/ramdisk.tmp" of="./ramdisk.bin" conv="notrunc"
	rm -rf /tmp/ramdisk.tmp /tmp/cpmd

################################################################################

bringup.o: bringup.c ramdisk.bin bringup.h
	$(CC) $(CFLAGS) -c -o ./$@ ./$<

################################################################################

%.o: %.c
	$(CC) $(CFLAGS) -c -o ./$@ ./$<

################################################################################

bios.o: bios.c
	$(CC) $(CFLAGS) -c -o ./$@ ./$<

################################################################################

rtc.o: rtc.c rtc.h
	$(CC) $(CFLAGS) -c -o ./$@ ./$<

################################################################################

pit.o: pit.c pit.h
	$(CC) $(CFLAGS) -c -o ./$@ ./$<

################################################################################

pmode.o: pmode.c pmode.h
	$(CC) $(CFLAGS) -c -o ./$@ ./$<

################################################################################

pmodeasm.o: pmode.s
	$(AS) $(ASFLAGS) $(NASMDEBUG) -I. -o ./$@ ./$<

################################################################################

mbentry.o: mbentry.s mltiboot.h
	$(AS) $(ASFLAGS) $(NASMDEBUG) -I. -o ./$@ ./$<

################################################################################

bss.inc: $(TARGET)
	$(NM) ./$(TARGET) | \
		$(AWK) '/__bss_start/ { print "bss_start equ 0x" $$1 }' > ./bss.inc
	$(NM) ./$(TARGET) | \
		$(AWK) '/__bss_end/ { print "bss_end equ 0x" $$1 }' >> ./bss.inc
	$(NM) ./$(TARGET) | \
		$(AWK) '/__kernel_end/ { print "kernel_end equ 0x" $$1 }' >> ./bss.inc
	$(NM) ./$(TARGET) | \
		$(AWK) '/ _start$$/ { print "kernel_entry equ 0x" $$1 }' >> ./bss.inc
	ke=$$($(NM) $(TARGET) | \
		$(AWK) '/__kernel_end/ { print $$1 }'); \
			bytes=$$((0x$$ke - 0x10000)); \
			sec=$$(( (bytes + 511) / 512 + 2 )); \
			$(PRINTF) '%s\n' "SECTORS_TO_LOAD equ $$sec" >> ./bss.inc

################################################################################

boot.bin: boot.s bss.inc
	$(AS) $(NASMDEBUG) -f bin -I. -o ./$@ ./$<

################################################################################

os.bin: $(TARGET)
	$(OBJCOPY) -O binary ./$(TARGET) ./$@

################################################################################

$(TARGET): $(OBJS) linker.ld
	$(LD) $(LMAP) $(LDFLAGS) -o ./$@ ./$(OBJS)
ifndef DEBUG
	$(STRIP) --strip-debug ./$(TARGET)
endif
	readelf -S ./$(TARGET) || :
	$(SIZE) ./$(TARGET) || :

################################################################################

floppy.img: boot.bin os.bin
	cat boot.bin os.bin > payload.bin
	$(DD) if="/dev/zero" of="$@" bs="1024" count="1440"
	$(DD) if="./payload.bin" of="$@" conv="notrunc"
	@set -- "$$(wc -c < ./payload.bin)"; pbytes="$$1"; \
	  if [ "$$pbytes" -gt 1474560 ]; then \
	    $(PRINTF) '%s\n' "ERROR: payload.bin $$pbytes >= 1474560 maximum"; \
	    exit 1; fi
	@$(PRINTF) '*** floppy.img usage: %d bytes\n' \
		"$$($(OD) -A x -t x2 ./floppy.img | $(GREP) -v '^*$$' | \
		tail -2 | head -1 | $(AWK) '{ print "0x"a$$1 }')" || :

################################################################################

.PHONY: clean distclean

clean distclean:
	rm -f ./*.o ./*.img /*.log ./*.bin ./*.386 \
		./$(MK386) ./$(SHOLE) ./$(TARGET)

################################################################################

testbdos: testbdos.c bdosmain.o bdosmisc.o bdosrw.o conbdos.o fileio.o \
	dskutil.o iosys.o ccp.o bringup.o
	$(CC) -m32 $(OPTFLAGS) -I. -o ./$@ ./$^

################################################################################

.PHONY: test

test: testbdos
	./testbdos

################################################################################

.PHONY: update-readme

update-readme: README.md
	"$${MAKE:-$(MAKE)}" markdown-toc
	"$${MAKE:-$(MAKE)}" scc

################################################################################

.PHONY: markdown-toc

markdown-toc: README.md
	markdown-toc -i ./README.md

################################################################################

.PHONY: scc

scc: README.md
	"$${MAKE:-$(MAKE)}" clean
	"$${MAKE:-$(MAKE)}" scc-real
	"$${MAKE:-$(MAKE)}" scc-real

################################################################################

.PHONY: scc-real

scc-real: README.md
	"$${MAKE:-$(MAKE)}" clean
	$(AWK) '/<!-- scc-start -->/ { \
		print; system("scc \
			--exclude-file LICENSE,README.awk,log.pvs \
			--exclude-file log.pvs,compile_commands.json \
			--exclude-file REUSE.toml \
			--exclude-dir LICENSES,.git,pvsreport,bindist \
			--no-cocomo -u --no-size -s lines -f html-table; \
			$(PRINTF) \"\n%s\n\" \"<!-- scc-end -->\""); \
			skip=1; next } \
		skip && /<!-- scc-end -->/ { skip=0; next } \
		!skip' ./README.md > ./README.awk && \
	mv -f ./README.awk ./README.md && \
	$(EXPAND) ./README.md > ./README.out && \
	mv -f ./README.out ./README.md

################################################################################

.PHONY: printvars printenv

printvars printenv:
	-@$(PRINTF) '%s: ' "FEATURES" 2> /dev/null
	-@$(PRINTF) '%s ' "$(.FEATURES)" 2> /dev/null
	-@$(PRINTF) '%s\n' "" 2> /dev/null
	-@$(foreach V,$(sort $(.VARIABLES)), \
	    $(if $(filter-out environment% default automatic,$(origin $V)), \
	    $(if $(strip $($V)),$(info $V: [$($V)]),)))
	-@true > /dev/null 2>&1

################################################################################

.PHONY: print-%

print-%:
	-@$(info $*: [$($*)] ($(flavor $*). set by $(origin $*)))@true
	-@true > /dev/null 2>&1

################################################################################
# Local Variables:
# mode: makefile
# indent-tabs-mode: t
# tab-width: 4
# whitespace-style: (tabs tab-mark)
# whitespace-display-mappings: ((tab-mark 9 [45] [45]))
# fill-column: 78
# eval: (setq-local whitespace-display-mappings
#                   '((tab-mark 9
#                               [45 45 62]
#                               [45 45 62])))
# eval: (whitespace-mode 1)
# eval: (setq-local display-fill-column-indicator-column 78)
# eval: (display-fill-column-indicator-mode 1)
# End:
################################################################################
# vim: set ft=make ts=4 ai noexpandtab list listchars=tab\:\>\- cc=80 :
################################################################################
