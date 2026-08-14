################################################################################
# CP/M-386 - GNUmakefile
# Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
# SPDX-License-Identifier: MIT
# scspell-id: b1bb2880-826f-11f1-a5c2-80ee73e9b8e7
################################################################################

################################################################################

export LC_ALL=C
export MAKE=$(shell printf '%s' \
	"$${MAKE:-$$(command -v gmake 2> /dev/null \
		|| command -v make 2> /dev/null || echo make)}")

################################################################################

CSTD:=-std=gnu89

################################################################################

ifndef DEBUG
 DEBUGFLAGS=-DNDEBUG
 LDEXTRA=
 NASMDEBUG=
 OPTFLAGS=-O2
else
 DEBUGFLAGS=-DDEBUG
 LDEXTRA=-Wl,--print-map -Wl,--cref
 NASMDEBUG=-g
 OPTFLAGS=-Og -ggdb -fdata-sections -ffunction-sections
endif

################################################################################

PRINTF:=$(shell \
	command -v gprintf 2> /dev/null || \
	command -v printf 2> /dev/null || \
	printf '%s' "printf")

################################################################################

DATE:=$(shell \
	command -v gdate 2> /dev/null || \
	command -v date 2> /dev/null || \
	$(PRINTF) '%s' "date")

################################################################################

CC:=$(shell \
	command -v gcc 2> /dev/null || \
	command -v clang 2> /dev/null || \
	command -v cc 2> /dev/null || \
	$(PRINTF) '%s' "cc")

################################################################################

NASM:=$(shell \
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

CP:=$(shell \
	command -v gcp 2> /dev/null || \
	command -v cp 2> /dev/null || \
	$(PRINTF) '%s' "cp")

################################################################################

MV:=$(shell \
	command -v gmv 2> /dev/null || \
	command -v mv 2> /dev/null || \
	$(PRINTF) '%s' "mv")

################################################################################

RM:=$(shell \
	command -v grm 2> /dev/null || \
	command -v rm 2> /dev/null || \
	$(PRINTF) '%s' "rm")

################################################################################

AWK:=$(shell \
	command -v gawk 2> /dev/null || \
	command -v goawk 2> /dev/null || \
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

TR:=$(shell \
	command -v gtr 2> /dev/null || \
	command -v tr 2> /dev/null || \
	$(PRINTF) '%s' "tr")

################################################################################

TAIL:=$(shell \
	command -v gtail 2> /dev/null || \
	command -v tail 2> /dev/null || \
	$(PRINTF) '%s' "tail")

################################################################################

HEAD:=$(shell \
	command -v ghead 2> /dev/null || \
	command -v head 2> /dev/null || \
	$(PRINTF) '%s' "head")

################################################################################

LZ4:=$(shell \
	command -v lz4 2> /dev/null || \
	$(PRINTF) '%s' "lz4")

################################################################################

GOBJ=$(shell $(OBJCOPY) --version 2>&1 | $(GREP) '^GNU objcopy' || :)
ifneq "$(findstring objcopy,$(GOBJ))" ""
 OBJCOPY+= -v
endif

################################################################################

GSTR=$(shell $(STRIP) --version 2>&1 | $(GREP) '^GNU strip' || :)
ifneq "$(findstring strip,$(GSTR))" ""
 STRIP+= -v
endif

################################################################################

GCP=$(shell $(CP) --version 2>&1 | $(GREP) 'GNU coreutils' || :)
ifneq "$(findstring coreutils,$(GCP))" ""
 CP+= -v
endif

################################################################################

GMV=$(shell $(MV) --version 2>&1 | $(GREP) 'GNU coreutils' || :)
ifneq "$(findstring coreutils,$(GMV))" ""
 MV+= -v
endif

################################################################################

GRM=$(shell $(RM) --version 2>&1 | $(GREP) 'GNU coreutils' || :)
ifneq "$(findstring coreutils,$(GRM))" ""
 RM+= -v
endif

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

W_NO_UNUSED_COMMAND_LINE_ARGUMENT:=$(shell \
	$(CC) -Werror -Wno-unused-command-line-argument \
	-x c -c /dev/null -o /dev/null 2> /dev/null && \
	$(PRINTF) '%s' "-Wno-unused-command-line-argument")

################################################################################

F_NO_STACK_CLASH_PROTECTION:=$(shell \
	$(CC) -Werror -fno-stack-clash-protection \
	-x c -c /dev/null -o /dev/null 2> /dev/null && \
	$(PRINTF) '%s' "-fno-stack-clash-protection")

################################################################################

F_NO_CF_PROTECTION:=$(shell \
	$(CC) -Werror -fcf-protection=none \
	-x c -c /dev/null -o /dev/null 2> /dev/null && \
	$(PRINTF) '%s' "-fcf-protection=none")

################################################################################

F_NO_SHORT_ENUMS:=$(shell \
	$(CC) -Werror -fno-short-enums \
	-x c -c /dev/null -o /dev/null 2> /dev/null && \
	$(PRINTF) '%s' "-fno-short-enums")

################################################################################

F_COLOR_DIAGNOSTICS:=$(shell \
	$(CC) -Werror -fcolor-diagnostics \
	-x c -c /dev/null -o /dev/null 2> /dev/null && \
	$(PRINTF) '%s' "-fcolor-diagnostics")

################################################################################

F_DIAGNOSTICS_COLOR:=$(shell \
	$(CC) -Werror -fdiagnostics-color=always \
	-x c -c /dev/null -o /dev/null 2> /dev/null && \
	$(PRINTF) '%s' "-fdiagnostics-color=always")

################################################################################

PIPE:=$(shell \
	$(CC) -Werror -pipe \
	-x c -c /dev/null -o /dev/null 2> /dev/null && \
	$(PRINTF) '%s' "-pipe")

################################################################################

DATESTR=$(shell env TZ=UTC date +"built %Y-%m-%d %H:%M:%S UTC")

################################################################################

LDEXTRA+= $(W_NO_UNUSED_COMMAND_LINE_ARGUMENT)

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
	$(CSTD) \
	$(DEBUGFLAGS) \
	$(F_COLOR_DIAGNOSTICS) \
	$(F_DIAGNOSTICS_COLOR) \
	$(F_NO_CF_PROTECTION) \
	$(F_NO_SHORT_ENUMS) \
	$(F_NO_STACK_CLASH_PROTECTION) \
	$(OPTFLAGS) \
	$(PIPE) \
	$(W_NO_DEPRECATED_NON_PROTOTYPE) \
	$(W_NO_RETURN_MISMATCH) \
	$(W_NO_UNUSED_COMMAND_LINE_ARGUMENT) \
	-DBUILDDATE='"$(DATESTR)"' \
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
	-fno-asynchronous-unwind-tables \
	-fno-builtin \
	-fno-pic \
	-fno-pie \
	-fno-plt \
	-fno-stack-protector \
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
	-Wl,--build-id=none \
	-Wno-implicit-function-declaration \
	-Wno-implicit-int \
	-Wno-incompatible-pointer-types \
	-Wno-int-conversion \
	-Wno-old-style-definition \
	-Wno-pointer-sign \
	-Wno-return-type \
	-Wno-sign-compare \
	-Wshadow

################################################################################

NASMFLAGS=-DBUILDDATE='"$(DATESTR)"' -f elf32
NASMFLAGS2=-DBUILDDATE='"$(DATESTR)"'

################################################################################

LDFLAGS = -m32 -nostdlib -Wl,-m,$(ELF_I386) -Wl,-T,linker.ld \
	-Wl,--build-id=none -Wl,--gc-sections

################################################################################

# Detect if CC with current CFLAGS supports `-flto`
ifndef NO_LTO
 FLTO_WR:=$(shell printf '%s\n' "int main(void){return 0;}" > .test.c; \
  $(CC) $(CFLAGS) -flto .test.c -o .test.out > /dev/null 2>&1; \
   echo $$?; $(RM) -f .test.c .test.out > /dev/null 2>&1)
 ifeq ($(FLTO_WR),0)
  FLTO_OK:=$(shell printf '%s\n' "int main(void){return 0;}" > .test.c; \
   $(CC) $(CFLAGS) -Werror -flto .test.c -o .test.out > /dev/null 2>&1; \
    echo $$?; $(RM) -f .test.c .test.out > /dev/null 2>&1)
  ifeq ($(FLTO_OK),0)
   LTO_FLAGS:=-flto
   # Detect if CC supports `-flto=auto`
   AUTO_WR:=$(shell printf '%s\n' "int main(void){return 0;}" > .test.c; \
    $(CC) $(CFLAGS) -flto=auto .test.c -o .test.out > /dev/null 2>&1; \
     echo $$?; $(RM) -f .test.c .test.out > /dev/null 2>&1)
   ifeq ($(AUTO_WR),0)
    AUTO_OK:=$(shell printf '%s\n' "int main(void){return 0;}" > .test.c; \
     $(CC) $(CFLAGS) -Werror -flto=auto .test.c -o .test.out > /dev/null 2>&1; \
      echo $$?; $(RM) -f .test.c .test.out > /dev/null 2>&1)
    ifeq ($(AUTO_OK),0)
     LTO_FLAGS:=-flto=auto
    endif
   endif
  endif
 endif
 ifeq ($(findstring -flto,$(CFLAGS)),)
  CFLAGS+=$(LTO_FLAGS)
 endif
 ifeq ($(findstring -flto,$(LDFLAGS)),)
  LDFLAGS+=$(LTO_FLAGS)
 endif
endif

################################################################################

# Solaris or illumos: Force `-flto=auto` to `-flto`
ifneq "$(findstring SunOS,$(OS))" ""
 CFLAGS := $(subst -flto=auto,-flto,$(CFLAGS))
 LDFLAGS := $(subst -flto=auto,-flto,$(LDFLAGS))
endif

################################################################################

# OS/400 with GCC: Disable LTO
ifneq "$(findstring OS400,$(OS))" ""
 ifneq "$(findstring gcc,$(CC))" ""
  CFLAGS:=$(subst -flto=auto,-flto,$(CFLAGS))
  CFLAGS:=$(subst -flto,,$(CFLAGS))
  LDFLAGS:=$(subst -flto=auto,-flto,$(LDFLAGS))
  LDFLAGS:=$(subst -flto,,$(LDFLAGS))
 endif
endif

################################################################################

BDOS_OBJS = bdosmain.o bdosmisc.o bdosrw.o conbdos.o fileio.o dskutil.o iosys.o
BIOS_OBJ = bios.o
BRINGUP_OBJ = bringup.o
CCP_OBJ = ccp.o
DISK_OBJ = disk.o
DISK_V86_OBJ = disk_v86.o
KBD_OBJ = kbd.o
MBENTRY_OBJ = mbentry.o
MEMMAP_OBJ = memmap.o
MLTIBOOT_OBJ = mltiboot.o
PIT_OBJ = pit.o
PMODE_OBJS = pmode.o pmodeasm.o
RNG_OBJ = cpmrng.o
RTC_OBJ = rtc.o
VGACON_OBJ = vgacon.o
VGATERM_OBJ = vgaterm.o
VIDBIOS_OBJS = vidbios.o vidbiosasm.o
VIDMODE_OBJ = vidmode.o

################################################################################

OBJS = $(BIOS_OBJ) $(BDOS_OBJS) $(CCP_OBJ) $(BRINGUP_OBJ) $(PMODE_OBJS) \
	$(MEMMAP_OBJ) $(VGACON_OBJ) $(VGATERM_OBJ) $(KBD_OBJ) $(VIDBIOS_OBJS) \
	$(VIDMODE_OBJ) $(RTC_OBJ) $(PIT_OBJ) $(RNG_OBJ) $(MBENTRY_OBJ) \
	$(MLTIBOOT_OBJ) $(DISK_OBJ) $(DISK_V86_OBJ)

################################################################################

TARGET = cpm386.elf

################################################################################

MK386 = mk386

################################################################################

MKLZ4RAW = mklz4raw

################################################################################

# Initial ramdisk image is 384 KiB (cpm386-384k)
RAMDISK_KB:=384

################################################################################

CPMFS:=cpm386-384k

################################################################################

.PHONY: all

all: $(TARGET) stage1.bin stage2.bin os.bin floppy.img disks
	@tput bold 2> /dev/null || :; tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "Build completed successfully."
	@tput sgr0 2> /dev/null || :
ifeq "$(findstring gcc,$(CC))" ""
	@tput bold 2> /dev/null || :; tput setaf 3 2> /dev/null || :
	@$(PRINTF) '%s\r\n' \
		"NOTE: $(CC) was used for this invocation but gcc is recommended!"
	@tput sgr0 2> /dev/null || :
endif

################################################################################

.PHONY: strip

strip: all
	@ls -l "./$(TARGET)" 2> /dev/null || :
	$(STRIP) -R '.strtab' -R '.symtab' "./$(TARGET)" 2> /dev/null || :
	@ls -l "./$(TARGET)" 2> /dev/null || :
	@tput bold 2> /dev/null || :; tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "Strip completed successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

%.bin: %.c user.ld
	$(CC) $(CFLAGS) -c -o ./$*.o ./$<
	$(CC) $(LDEXTRA) $(LTO_FLAGS) -Wl,--build-id=none -nostdlib \
		-Wl,-m,$(ELF_I386) -no-pie -Wl,-T,./user.ld -o ./$*.elf ./$*.o
	@entry=$$($(NM) ./$*.elf | $(AWK) '$$NF=="_start"{print $$1}'); \
	  if [ "$$entry" != "00000100" ]; then \
	    tput bold 2> /dev/null || :; tput setaf 1 2> /dev/null || :; \
	    $(PRINTF) '%s\n' "ERROR: _start at 0x$$entry, expected 0x100"; \
	    $(NM) ./$*.elf | head -20; \
	    tput sgr0 2> /dev/null || :; \
	    exit 1; \
	  fi
	$(OBJCOPY) -O binary ./$*.elf ./$@
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

$(MK386): mk386.c
	$(CC) $(CSTD) $(OPTFLAGS) $(LTO_FLAGS) -Wl,--build-id=none -o ./$@ ./$<
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

hello.386: hello.bin $(MK386)
	./$(MK386) ./hello.bin ./hello.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

test110.386: test110.bin $(MK386)
	./$(MK386) ./test110.bin ./test110.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

test211.386: test211.bin $(MK386)
	./$(MK386) ./test211.bin ./test211.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

stat.386: stat.bin $(MK386)
	./$(MK386) ./stat.bin ./stat.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

lrbc.386: lrbc.bin $(MK386)
	./$(MK386) ./lrbc.bin ./lrbc.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

iotest.386: iotest.bin $(MK386)
	./$(MK386) ./iotest.bin ./iotest.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

delay.386: delay.bin $(MK386)
	./$(MK386) ./delay.bin ./delay.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

gfxtest.386: gfxtest.bin $(MK386)
	./$(MK386) ./gfxtest.bin ./gfxtest.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

getsn.386: getsn.bin $(MK386)
	./$(MK386) ./getsn.bin ./getsn.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

sync.386: sync.bin $(MK386)
	./$(MK386) ./sync.bin ./sync.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

tod.386: tod.bin $(MK386)
	./$(MK386) ./tod.bin ./tod.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

hd.386: hd.bin $(MK386)
	./$(MK386) ./hd.bin ./hd.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

od.386: od.bin $(MK386)
	./$(MK386) ./od.bin ./od.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

ls.386: ls.bin $(MK386)
	./$(MK386) ./ls.bin ./ls.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

mandel.386: mandel.bin $(MK386)
	./$(MK386) ./mandel.bin ./mandel.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

julia.386: julia.bin $(MK386)
	./$(MK386) ./julia.bin ./julia.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

ver.386: ver.bin $(MK386)
	./$(MK386) ./ver.bin ./ver.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

ed.386: ed.bin $(MK386)
	./$(MK386) ./ed.bin ./ed.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

pip.386: pip.bin $(MK386)
	./$(MK386) ./pip.bin ./pip.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

reboot.386: reboot.bin $(MK386)
	./$(MK386) ./reboot.bin ./reboot.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

touch.386: touch.bin $(MK386)
	./$(MK386) ./touch.bin ./touch.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

more.386: more.bin $(MK386)
	./$(MK386) ./more.bin ./more.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

cls.386: cls.bin $(MK386)
	./$(MK386) cls.bin cls.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

rc.386: rc.bin $(MK386)
	./$(MK386) ./rc.bin ./rc.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

tsec.386: tsec.bin $(MK386)
	./$(MK386) ./tsec.bin ./tsec.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

alvtst.386: alvtst.bin $(MK386)
	./$(MK386) ./alvtst.bin ./alvtst.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

trunctst.386: trunctst.bin $(MK386)
	./$(MK386) ./trunctst.bin ./trunctst.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

truncate.386: truncate.bin $(MK386)
	./$(MK386) ./truncate.bin ./truncate.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

rm.386: rm.bin $(MK386)
	./$(MK386) ./rm.bin ./rm.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

printenv.386: printenv.bin $(MK386)
	./$(MK386) ./printenv.bin ./printenv.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

cleartpa.386: cleartpa.bin $(MK386)
	./$(MK386) ./cleartpa.bin ./cleartpa.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

pause.386: pause.bin $(MK386)
	./$(MK386) ./pause.bin ./pause.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

vgaon.bin: conctl.c user.ld
	$(CC) $(CFLAGS) -DPROG_VGAON -c -o ./vgaon.o ./conctl.c
	$(CC) $(LDEXTRA) $(LTO_FLAGS) -Wl,--build-id=none -nostdlib \
		-Wl,-m,$(ELF_I386) -no-pie -Wl,-T,./user.ld -o ./vgaon.elf ./vgaon.o
	@entry=$$($(NM) ./vgaon.elf | $(AWK) '$$NF=="_start"{print $$1}'); \
	  if [ "$$entry" != "00000100" ]; then \
	    tput bold 2> /dev/null || :; tput setaf 1 2> /dev/null || :; \
	    $(PRINTF) '%s\n' "ERROR: vgaon _start"; \
	    tput sgr0 2> /dev/null || :; \
	    exit 1; \
	  fi
	$(OBJCOPY) -O binary ./vgaon.elf ./$@
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

vgaoff.bin: conctl.c user.ld
	$(CC) $(CFLAGS) -DPROG_VGAOFF -c -o ./vgaoff.o ./conctl.c
	$(CC) $(LDEXTRA) $(LTO_FLAGS) -Wl,--build-id=none -nostdlib \
		-Wl,-m,$(ELF_I386) -no-pie -Wl,-T,./user.ld -o ./vgaoff.elf ./vgaoff.o
	@entry=$$($(NM) ./vgaoff.elf | $(AWK) '$$NF=="_start"{print $$1}'); \
	  if [ "$$entry" != "00000100" ]; then \
	    tput bold 2> /dev/null || :; tput setaf 1 2> /dev/null || :; \
	    $(PRINTF) '%s\n' "ERROR: vgaoff _start"; \
	    tput sgr0 2> /dev/null || :; \
	    exit 1; \
	  fi
	$(OBJCOPY) -O binary ./vgaoff.elf ./$@
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

seron.bin: conctl.c user.ld
	$(CC) $(CFLAGS) -DPROG_SERON -c -o ./seron.o ./conctl.c
	$(CC) $(LDEXTRA) $(LTO_FLAGS) -Wl,--build-id=none -nostdlib \
		-Wl,-m,$(ELF_I386) -no-pie -Wl,-T,./user.ld -o ./seron.elf ./seron.o
	@entry=$$($(NM) ./seron.elf | $(AWK) '$$NF=="_start"{print $$1}'); \
	  if [ "$$entry" != "00000100" ]; then \
	    tput bold 2> /dev/null || :; tput setaf 1 2> /dev/null || :; \
	    $(PRINTF) '%s\n' "ERROR: seron _start"; \
	    tput sgr0 2> /dev/null || :; \
	    exit 1; \
	  fi
	$(OBJCOPY) -O binary ./seron.elf ./$@
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

seroff.bin: conctl.c user.ld
	$(CC) $(CFLAGS) -DPROG_SEROFF -c -o ./seroff.o ./conctl.c
	$(CC) $(LDEXTRA) $(LTO_FLAGS) -Wl,--build-id=none -nostdlib \
		-Wl,-m,$(ELF_I386) -no-pie -Wl,-T,./user.ld -o ./seroff.elf ./seroff.o
	@entry=$$($(NM) ./seroff.elf | $(AWK) '$$NF=="_start"{print $$1}'); \
	  if [ "$$entry" != "00000100" ]; then \
	    tput bold 2> /dev/null || :; tput setaf 1 2> /dev/null || :; \
	    $(PRINTF) '%s\n' "ERROR: seroff _start"; \
	    tput sgr0 2> /dev/null || :; \
	    exit 1; \
	  fi
	$(OBJCOPY) -O binary ./seroff.elf ./$@
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

vgaon.386: vgaon.bin $(MK386)
	./$(MK386) ./vgaon.bin ./vgaon.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

vgaoff.386: vgaoff.bin $(MK386)
	./$(MK386) ./vgaoff.bin ./vgaoff.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

seron.386: seron.bin $(MK386)
	./$(MK386) ./seron.bin ./seron.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

seroff.386: seroff.bin $(MK386)
	./$(MK386) ./seroff.bin ./seroff.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

fparse.386: fparse.bin $(MK386)
	./$(MK386) ./fparse.bin ./fparse.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

illegal.386: illegal.bin $(MK386)
	./$(MK386) ./illegal.bin ./illegal.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

dumpfcb.386: dumpfcb.bin $(MK386)
	./$(MK386) ./dumpfcb.bin ./dumpfcb.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

dumpdir.386: dumpdir.bin $(MK386)
	./$(MK386) ./dumpdir.bin ./dumpdir.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

mem.386: mem.bin $(MK386)
	./$(MK386) ./mem.bin ./mem.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

aclockvt.386: aclockvt.bin $(MK386)
	./$(MK386) ./aclockvt.bin ./aclockvt.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

aclockdv.386: aclockdv.bin $(MK386)
	./$(MK386) ./aclockdv.bin ./aclockdv.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

vgafont.386: vgafont.bin $(MK386)
	./$(MK386) ./vgafont.bin ./vgafont.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

vgatext.386: vgatext.bin $(MK386)
	./$(MK386) ./vgatext.bin ./vgatext.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

capslock.386: capslock.bin $(MK386)
	./$(MK386) ./capslock.bin ./capslock.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

esctilde.386: esctilde.bin $(MK386)
	./$(MK386) ./esctilde.bin ./esctilde.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

numlock.386: numlock.bin $(MK386)
	./$(MK386) ./numlock.bin ./numlock.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

termtest.386: termtest.bin $(MK386)
	./$(MK386) ./termtest.bin ./termtest.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

textmode.386: textmode.bin $(MK386)
	./$(MK386) ./textmode.bin ./textmode.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

ticks.386: ticks.bin $(MK386)
	./$(MK386) ./ticks.bin ./ticks.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

prng.386: prng.bin $(MK386)
	./$(MK386) ./prng.bin ./prng.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

ramdisk.bin: \
			aclockdv.386 \
			aclockvt.386 \
			alvtst.386 \
			capslock.386 \
			cleartpa.386 \
			cls.386 \
			delay.386 \
			dumpdir.386 \
			dumpfcb.386 \
			ed.386 \
			esctilde.386 \
			fparse.386 \
			getsn.386 \
			gfxtest.386 \
			hd.386 \
			hello.386 \
			illegal.386 \
			iotest.386 \
			julia.386 \
			lrbc.386 \
			ls.386 \
			mandel.386 \
			mem.386 \
			more.386 \
			numlock.386 \
			od.386 \
			pause.386 \
			pip.386 \
			printenv.386 \
			prng.386 \
			rc.386 \
			reboot.386 \
			rm.386 \
			seroff.386 \
			seron.386 \
			stat.386 \
			sync.386 \
			termtest.386 \
			test110.386 \
			test211.386 \
			textmode.386 \
			ticks.386 \
			tod.386 \
			touch.386 \
			truncate.386 \
			trunctst.386 \
			tsec.386 \
			ver.386 \
			vgafont.386 \
			vgaoff.386 \
			vgaon.386 \
			vgatext.386
	$(RM) -f ./ramdisk.tmp
	$(RM) -rf ./.cpmd
	mkdir -p ./.cpmd
	$(PRINTF) 'This is README.TXT from the CP/M-386 RAM disk.\r\n\x1a' \
		> ./.cpmd/README.TXT
	$(PRINTF) '; Sample ENV.DAT for CP/M-386\r\nHELLO=World\r\n\x1a' \
		> ./.cpmd/ENV.DAT
	$(PRINTF) '; Hello from ANOTHER1.SUB\r\n\x1a' \
		> ./.cpmd/ANOTHER1.SUB
	$(PRINTF) '; Hello from ANOTHER.SUB\r\nANOTHER1.SUB\r\n; Back in ANOTHER.SUB\r\n\x1a' \
		> ./.cpmd/ANOTHER.SUB
	$(PRINTF) '; DEMO.SUB - SUBMIT on CP/M-386 testing\r\n; Nested submit\r\nANOTHER.SUB\r\n; Back in DEMO.SUB\r\nSEROFF\r\nSERON\r\nVGAOFF\r\nVGAON\r\nVER\r\nGETSN\r\nMEM\r\nTOD\r\nTSEC\r\nTICKS\r\nTOD\r\nTSEC\r\nTICKS\r\nTOD\r\nTSEC\r\nTICKS\r\nDELAY\r\nSTAT DSK:\r\nSTAT STAT.386 SIZE\r\nPRINTENV\r\nTEST110\r\nTEST211\r\nLS -A\r\nDIR\r\nLS -L STAT.*\r\nLRBC STAT.386\r\nTRUNCTST\r\nALVTST\r\nIOTEST\r\nSYNC\r\nFPARSE\r\nILLEGAL\r\nLRBC README.TXT\r\nTOUCH NEW.DAT\r\nERA NEW.DAT\r\nDUMPFCB DEMO.SUB\r\nDUMPDIR DEMO.*\r\nHD CLS.386\r\nOD CLS.386\r\nERA TRUNC.DAT\r\nRC 1\r\nREN IOWORK.D4T=IOWORK.DAT\r\nRM IOWORK.D4T\r\nRC\r\nHELLO\r\nPRNG 128\r\n; Run PRNG again from image existing in TPA\r\nGO 64\r\n; End of DEMO.SUB\r\n\x1a' \
		> ./.cpmd/DEMO.SUB
	$(PRINTF) '@QUIET ON\r\nVER\r\n\x1a' > ./.cpmd/PROFILE.SUB
	$(CP) -f ./aclockdv.386 ./.cpmd/ACLOCKDV.386
	$(CP) -f ./aclockvt.386 ./.cpmd/ACLOCKVT.386
	$(CP) -f ./alvtst.386 ./.cpmd/ALVTST.386
	$(CP) -f ./capslock.386 ./.cpmd/CAPSLOCK.386
	$(CP) -f ./cleartpa.386 ./.cpmd/CLEARTPA.386
	$(CP) -f ./cls.386 ./.cpmd/CLS.386
	$(CP) -f ./delay.386 ./.cpmd/DELAY.386
	$(CP) -f ./dumpdir.386 ./.cpmd/DUMPDIR.386
	$(CP) -f ./dumpfcb.386 ./.cpmd/DUMPFCB.386
	$(CP) -f ./ed.386 ./.cpmd/ED.386
	$(CP) -f ./esctilde.386 ./.cpmd/ESCTILDE.386
	$(CP) -f ./fparse.386 ./.cpmd/FPARSE.386
	$(CP) -f ./getsn.386 ./.cpmd/GETSN.386
	$(CP) -f ./gfxtest.386 ./.cpmd/GFXTEST.386
	$(CP) -f ./hd.386 ./.cpmd/HD.386
	$(CP) -f ./hello.386 ./.cpmd/HELLO.386
	$(CP) -f ./illegal.386 ./.cpmd/ILLEGAL.386
	$(CP) -f ./iotest.386 ./.cpmd/IOTEST.386
	$(CP) -f ./julia.386 ./.cpmd/JULIA.386
	$(CP) -f ./lrbc.386 ./.cpmd/LRBC.386
	$(CP) -f ./ls.386 ./.cpmd/LS.386
	$(CP) -f ./mandel.386 ./.cpmd/MANDEL.386
	$(CP) -f ./mem.386 ./.cpmd/MEM.386
	$(CP) -f ./more.386 ./.cpmd/MORE.386
	$(CP) -f ./numlock.386 ./.cpmd/NUMLOCK.386
	$(CP) -f ./od.386 ./.cpmd/OD.386
	$(CP) -f ./pause.386 ./.cpmd/PAUSE.386
	$(CP) -f ./pip.386 ./.cpmd/PIP.386
	$(CP) -f ./printenv.386 ./.cpmd/PRINTENV.386
	$(CP) -f ./prng.386 ./.cpmd/PRNG.386
	$(CP) -f ./rc.386 ./.cpmd/RC.386
	$(CP) -f ./reboot.386 ./.cpmd/REBOOT.386
	$(CP) -f ./rm.386 ./.cpmd/RM.386
	$(CP) -f ./seroff.386 ./.cpmd/SEROFF.386
	$(CP) -f ./seron.386 ./.cpmd/SERON.386
	$(CP) -f ./stat.386 ./.cpmd/STAT.386
	$(CP) -f ./sync.386 ./.cpmd/SYNC.386
	$(CP) -f ./termtest.386 ./.cpmd/TERMTEST.386
	$(CP) -f ./test110.386 ./.cpmd/TEST110.386
	$(CP) -f ./test211.386 ./.cpmd/TEST211.386
	$(CP) -f ./textmode.386 ./.cpmd/TEXTMODE.386
	$(CP) -f ./ticks.386 ./.cpmd/TICKS.386
	$(CP) -f ./tod.386 ./.cpmd/TOD.386
	$(CP) -f ./touch.386 ./.cpmd/TOUCH.386
	$(CP) -f ./truncate.386 ./.cpmd/TRUNCATE.386
	$(CP) -f ./trunctst.386 ./.cpmd/TRUNCTST.386
	$(CP) -f ./tsec.386 ./.cpmd/TSEC.386
	$(CP) -f ./ver.386 ./.cpmd/VER.386
	$(CP) -f ./vgafont.386 ./.cpmd/VGAFONT.386
	$(CP) -f ./vgaoff.386 ./.cpmd/VGAOFF.386
	$(CP) -f ./vgaon.386 ./.cpmd/VGAON.386
	$(CP) -f ./vgatext.386 ./.cpmd/VGATEXT.386
	mkfs.cpm -f $(CPMFS) ./ramdisk.tmp
	cpmcp -f $(CPMFS) ./ramdisk.tmp \
	  ./.cpmd/ACLOCKDV.386 \
	  ./.cpmd/ACLOCKVT.386 \
	  ./.cpmd/ALVTST.386 \
	  ./.cpmd/ANOTHER1.SUB \
	  ./.cpmd/ANOTHER.SUB \
	  ./.cpmd/CAPSLOCK.386 \
	  ./.cpmd/CLEARTPA.386 \
	  ./.cpmd/CLS.386 \
	  ./.cpmd/DELAY.386 \
	  ./.cpmd/DEMO.SUB \
	  ./.cpmd/DUMPDIR.386 \
	  ./.cpmd/DUMPFCB.386 \
	  ./.cpmd/ED.386 \
	  ./.cpmd/ENV.DAT \
	  ./.cpmd/ESCTILDE.386 \
	  ./.cpmd/FPARSE.386 \
	  ./.cpmd/GETSN.386 \
	  ./.cpmd/GFXTEST.386 \
	  ./.cpmd/HD.386 \
	  ./.cpmd/HELLO.386 \
	  ./.cpmd/ILLEGAL.386 \
	  ./.cpmd/IOTEST.386 \
	  ./.cpmd/JULIA.386 \
	  ./.cpmd/LRBC.386 \
	  ./.cpmd/LS.386 \
	  ./.cpmd/MANDEL.386 \
	  ./.cpmd/MEM.386 \
	  ./.cpmd/MORE.386 \
	  ./.cpmd/NUMLOCK.386 \
	  ./.cpmd/OD.386 \
	  ./.cpmd/PAUSE.386 \
	  ./.cpmd/PIP.386 \
	  ./.cpmd/PRINTENV.386 \
	  ./.cpmd/PRNG.386 \
	  ./.cpmd/PROFILE.SUB \
	  ./.cpmd/RC.386 \
	  ./.cpmd/README.TXT \
	  ./.cpmd/REBOOT.386 \
	  ./.cpmd/RM.386 \
	  ./.cpmd/SEROFF.386 \
	  ./.cpmd/SERON.386 \
	  ./.cpmd/STAT.386 \
	  ./.cpmd/SYNC.386 \
	  ./.cpmd/TERMTEST.386 \
	  ./.cpmd/TEST110.386 \
	  ./.cpmd/TEST211.386 \
	  ./.cpmd/TEXTMODE.386 \
	  ./.cpmd/TICKS.386 \
	  ./.cpmd/TOD.386 \
	  ./.cpmd/TOUCH.386 \
	  ./.cpmd/TRUNCATE.386 \
	  ./.cpmd/TRUNCTST.386 \
	  ./.cpmd/TSEC.386 \
	  ./.cpmd/VER.386 \
	  ./.cpmd/VGAFONT.386 \
	  ./.cpmd/VGAOFF.386 \
	  ./.cpmd/VGAON.386 \
	  ./.cpmd/VGATEXT.386 \
	  0:
	@RDS=$$($(PRINTF) '%d' "$$($(OD) -A x -t x2 ./ramdisk.tmp | \
		$(GREP) -v '^*$$' | $(TAIL) -2 | $(HEAD) -1 | \
		$(AWK) '{ print "0x"a$$1 }')"); \
		RDS=$$(( (RDS / 1024) + 4 )); \
		tput setaf 6 2> /dev/null || :; \
		$(PRINTF) '*** ramdisk.tmp usage: ~%s KiB\n' "$$(( RDS - 4 ))"; \
		tput sgr0 2> /dev/null || :; \
		test "$$RDS" -lt "$(RAMDISK_KB)" || { \
		tput bold 2> /dev/null || :; tput setaf 1 2> /dev/null || :; \
		$(PRINTF) '%s\n' \
			"*** ERROR: ramdisk too large for $(RAMDISK_KB)K space reserved"; \
			tput sgr0 2> /dev/null || :; \
			exit 2; }
	$(DD) if="/dev/zero" of="./ramdisk.bin" bs="1024" count="$(RAMDISK_KB)" \
		2>&1
	$(DD) if="./ramdisk.tmp" of="./ramdisk.bin" conv="notrunc" \
		2>&1
	$(RM) -rf ./ramdisk.tmp ./.cpmd
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

$(BRINGUP_OBJ): bringup.c ramdisk.bin bringup.h bdosinc.h biosdef.h bdosdef.h
	$(CC) $(CFLAGS) -c -o ./$@ ./$<
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

%.o: %.c
	$(CC) $(CFLAGS) -c -o ./$@ ./$<
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

$(BIOS_OBJ): bios.c bdosinc.h bdosdef.h biosdef.h bringup.h pmode.h \
		absaddr.h memmap.h io.h vgacon.h vgaterm.h kbd.h disk.h
	$(CC) $(CFLAGS) -c -o ./$@ ./$<
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

$(MEMMAP_OBJ): memmap.c memmap.h absaddr.h
	$(CC) $(CFLAGS) -c -o ./$@ ./$<
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

$(VGACON_OBJ): vgacon.c vgacon.h io.h absaddr.h platform.h
	$(CC) $(CFLAGS) -c -o ./$@ ./$<
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

$(VGATERM_OBJ): vgaterm.c vgaterm.h vgacon.h kbd.h
	$(CC) $(CFLAGS) -c -o ./$@ ./$<
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

$(KBD_OBJ): kbd.c kbd.h io.h absaddr.h
	$(CC) $(CFLAGS) -c -o ./$@ ./$<
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

vidbios.o: vidbios.c vidbios.h absaddr.h
	$(CC) $(CFLAGS) -c -o ./$@ ./$<
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

$(VIDMODE_OBJ): vidmode.c vidmode.h vidbios.h vgacon.h absaddr.h
	$(CC) $(CFLAGS) -c -o ./$@ ./$<
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

vidbiosasm.o: vidbios.s
	$(NASM) $(NASMFLAGS) $(NASMDEBUG) -I. -l ./${@:.o=.lst} -o ./$@ ./$<
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

$(MLTIBOOT_OBJ): mltiboot.c mltiboot.h memmap.h absaddr.h bdosinc.h biosdef.h
	$(CC) $(CFLAGS) -c -o ./$@ ./$<
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

# The BDOS sources share one set of headers; bdosdef.h carries the ring-3
# request structs (cpm_vga_text / cpm_ticks / cpm_memlayout).

BDOS_HDRS = bdosinc.h bdosdef.h biosdef.h pktio.h platform.h vidmode.h vgacon.h

bdosmain.o: bdosmain.c $(BDOS_HDRS)
bdosmisc.o: bdosmisc.c $(BDOS_HDRS)
bdosrw.o: bdosrw.c $(BDOS_HDRS)
conbdos.o: conbdos.c $(BDOS_HDRS)
fileio.o: fileio.c $(BDOS_HDRS)
dskutil.o: dskutil.c $(BDOS_HDRS)
iosys.o: iosys.c $(BDOS_HDRS)

################################################################################

$(CCP_OBJ): ccp.c ccpdef.h bdosinc.h bdosdef.h

################################################################################

$(RTC_OBJ): rtc.c rtc.h
	$(CC) $(CFLAGS) -c -o ./$@ ./$<
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

$(PIT_OBJ): pit.c pit.h
	$(CC) $(CFLAGS) -c -o ./$@ ./$<
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

$(RNG_OBJ): cpmrng.c cpmrng.h
	$(CC) $(CFLAGS) -c -o ./$@ ./$<
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

pmode.o: pmode.c pmode.h bdosinc.h platform.h io.h vidbios.h vidmode.h
	$(CC) $(CFLAGS) -c -o ./$@ ./$<
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

$(DISK_V86_OBJ): disk_v86.s
	$(NASM) $(NASMFLAGS) $(NASMDEBUG) -I. -l ./${@:.o=.lst} -o ./$@ ./$<
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

$(DISK_OBJ): disk.c disk.h bringup.h bdosinc.h biosdef.h io.h pit.h pmode.h
	$(CC) $(CFLAGS) -c -o ./$@ ./$<
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

pmodeasm.o: pmode.s
	$(NASM) $(NASMFLAGS) $(NASMDEBUG) -I. -l ./${@:.o=.lst} -o ./$@ ./$<
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

$(MBENTRY_OBJ): mbentry.s mltiboot.h
	$(NASM) $(NASMFLAGS) $(NASMDEBUG) -I. -l ./${@:.o=.lst} -o ./$@ ./$<
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

bss.inc: $(TARGET) os.bin.lz4raw
	$(NM) ./$(TARGET) 2>&1 | $(GREP) -q "no symbols" 2> /dev/null && { \
		$(RM) -f ./$(TARGET) && "$${MAKE:-$(MAKE)}" $(TARGET); } || :
	$(NM) ./$(TARGET) | \
		$(AWK) '/__bss_start/ { print "bss_start equ 0x" $$1 }' > ./bss.inc
	$(NM) ./$(TARGET) | \
		$(AWK) '/__bss_end/ { print "bss_end equ 0x" $$1 }' >> ./bss.inc
	$(NM) ./$(TARGET) | \
		$(AWK) '/__kernel_end/ { print "kernel_end equ 0x" $$1 }' >> ./bss.inc
	$(NM) ./$(TARGET) | \
		$(AWK) '/ _start$$/ { print "kernel_entry equ 0x" $$1 }' >> ./bss.inc
	cbytes=$$(wc -c < ./os.bin.lz4raw | tr -d ' '); \
		sec=$$(( (cbytes + 511) / 512 + 2 )); \
		$(PRINTF) '%s\n' "SECTORS_TO_LOAD equ $$sec" >> ./bss.inc; \
		$(PRINTF) '%s\n' "COMPRESSED_SIZE equ $$cbytes" >> ./bss.inc
	@ke=$$($(NM) $(TARGET) | \
		$(AWK) '/__kernel_end/ { print $$1 }'); \
		top=$$((0x$$ke + 0x4000)); \
		if [ "$$top" -gt $$((0x9E000)) ]; then \
		  tput bold 2> /dev/null || :; tput setaf 1 2> /dev/null || :; \
		  $(PRINTF) '%s' \
		    "ERROR: kernel end + r0 stack ($$top) overruns conventional"; \
		  $(PRINTF) '%s\n' " memory (0x9E000)!"; \
		  tput sgr0 2> /dev/null || :; \
		  exit 1; \
		fi
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

stage1.bin: stage1.s layout.inc
	$(NASM) $(NASMFLAGS2) $(NASMDEBUG) -f bin \
		-I. -l ./${@:.bin=.lst} -o ./$@ ./$<
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

stage2.bin: stage2.s layout.inc bss.inc lz4dec.inc
	$(NM) ./$(TARGET) 2>&1 | $(GREP) -q "no symbols" 2> /dev/null && { \
		$(RM) -f ./$(TARGET) && "$${MAKE:-$(MAKE)}" $(TARGET); } || :
	$(NASM) $(NASMFLAGS2) $(NASMDEBUG) -f bin \
		-I. -l ./${@:.bin=.lst} -o ./$@ ./$<
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

os.bin: $(TARGET)
	$(OBJCOPY) -O binary ./$(TARGET) ./$@
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

$(MKLZ4RAW): mklz4raw.c
	$(CC) $(CSTD) $(OPTFLAGS) $(LTO_FLAGS) -Wl,--build-id=none -o ./$@ ./$<
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

os.bin.lz4: os.bin
	$(LZ4) --best --favor-decSpeed --no-frame-crc -k -f -q ./$< ./$@
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

os.bin.lz4raw: os.bin.lz4 $(MKLZ4RAW)
	./$(MKLZ4RAW) ./$< ./$@
	@tput setaf 6 2> /dev/null || :
	@$(PRINTF) '*** lz4 compression: %d bytes -> %d bytes (~%.1f%%)\n' \
	 $$(wc -c < ./os.bin) $$(wc -c < ./$@) \
	  $$($(AWK) \
	   'BEGIN{printf "%.1f",('$$(wc -c < ./$@)'*100.0/'$$(wc -c < ./os.bin)')}')
	@tput sgr0 2> /dev/null || :
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

$(TARGET): $(OBJS) linker.ld
	$(CC) $(LDEXTRA) $(LDFLAGS) -o ./$@ ./$(OBJS)
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

floppy.img: stage1.bin stage2.bin os.bin.lz4raw
	cat ./stage1.bin ./stage2.bin ./os.bin.lz4raw > ./payload.bin
	$(PRINTF) '\x1a' >> ./payload.bin
	$(DD) if="/dev/zero" of="$@" bs="1024" count="1440" 2>&1
	$(DD) if="./payload.bin" of="$@" conv="notrunc" 2>&1
	@set -- "$$(wc -c < ./payload.bin)"; pbytes="$$1"; \
	  if [ "$$pbytes" -gt 1474560 ]; then \
	    tput bold 2> /dev/null || :; tput setaf 1 2> /dev/null || :; \
	    $(PRINTF) '%s\n' "ERROR: payload.bin $$pbytes >= 1474560 maximum"; \
	    tput sgr0 2> /dev/null || :; \
	    exit 1; \
	  fi
	@tput setaf 6 2> /dev/null || :
	@$(PRINTF) '*** floppy.img usage: ~%d KiB\n' \
		$$(("$$($(OD) -A x -t x2 ./floppy.img | $(GREP) -v '^*$$' | \
		$(TAIL) -2 | $(HEAD) -1 | $(AWK) '{ print "0x"a$$1 }')" / 1024)) || :
	@tput sgr0 2> /dev/null || :
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

.PHONY: disks

disks: hd.img fd.img

################################################################################

hd.img: diskdefs
	$(DD) if="/dev/zero" bs="1024" count="8192" 2>/dev/null | \
		$(TR) '\0' '\345' > "$@"
	mkfs.cpm -f cpm386-hd8 "$@"
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

fd.img: diskdefs
	$(DD) if="/dev/zero" bs="1024" count="1440" 2>/dev/null | \
		$(TR) '\0' '\345' > "$@"
	mkfs.cpm -f cpm386-fd144 "$@"
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

.PHONY: clean distclean

clean distclean:
	$(RM) -f ./*.o ./*.elf ./*.img /*.log ./*.bin ./*.386 ./*.lst \
		./bss.inc ./testbdos ./*.su ./*.ci ./$(MK386) ./$(MKLZ4RAW) \
		./*.lz4 ./*.lz4raw ramdisk.tmp ./$(TARGET)
	$(RM) -f -r ./.cpmd
	@ccache -cC > /dev/null 2>&1 || :
	@tput bold 2> /dev/null || :; tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "Clean completed successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

testbdos: testbdos.c bdosmain.o bdosmisc.o bdosrw.o conbdos.o fileio.o \
	dskutil.o iosys.o $(CCP_OBJ) $(BRINGUP_OBJ)
	$(CC) $(CSTD) -m32 $(LTO_FLAGS) $(OPTFLAGS) -Wl,--build-id=none -I. \
	-o ./$@ ./$^ -DRAMDISK_KB=$(RAMDISK_KB) -Wl,--build-id=none \
	-fno-asynchronous-unwind-tables -fno-builtin -fno-pic -fno-pie -fno-plt \
	-fno-stack-protector -fno-unwind-tables -fomit-frame-pointer $(TEST_FLAGS)
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

.PHONY: lint

lint:
	cppi -ac ./*.[ch]
	reuse lint -q || reuse lint
	shellcheck -o any,all ./*.sh
	shfmt -bn -sr -fn -i 2 -s -d ./*.sh
	@tput bold 2> /dev/null || :; tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "Lint completed successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

.PHONY: test

test: testbdos
	./testbdos
	@tput bold 2> /dev/null || :; tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "Tests completed successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

.PHONY: update-readme

update-readme: README.md
	"$${MAKE:-$(MAKE)}" markdown-toc
	"$${MAKE:-$(MAKE)}" scc
	@tput bold 2> /dev/null || :; tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "update-readme completed successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

.PHONY: markdown-toc

markdown-toc: README.md
	markdown-toc -i ./README.md
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "markdown-toc completed successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

.PHONY: scc

scc: README.md
	"$${MAKE:-$(MAKE)}" clean
	"$${MAKE:-$(MAKE)}" scc-real
	"$${MAKE:-$(MAKE)}" scc-real
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "scc completed successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

.PHONY: scc-real

scc-real: README.md
	"$${MAKE:-$(MAKE)}" clean
	$(AWK) '/<!-- scc-start -->/ { \
		print; system("scc \
			--count-as-pattern \"BSDmakefile:Makefile:Makefile\" \
			--count-as-pattern \"*.ld:Linker&nbsp;Script:Text\" \
			--count-as-pattern \"*.inc:Assembly:Assembly\" \
			--exclude-file LICENSE,README.awk,log.pvs \
			--exclude-file log.pvs,compile_commands.json \
			--exclude-file REUSE.toml \
			--exclude-dir LICENSES,.git,pvsreport,bindist \
			--no-cocomo -u --no-size -s lines -f html-table; \
			$(PRINTF) \"\n%s\n\" \"<!-- scc-end -->\""); \
			skip=1; next } \
		skip && /<!-- scc-end -->/ { skip=0; next } \
		!skip' ./README.md > ./README.awk && \
	$(MV) -f ./README.awk ./README.md && \
	$(EXPAND) ./README.md > ./README.out && \
	$(MV) -f ./README.out ./README.md
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "scc-real completed successfully."
	@tput sgr0 2> /dev/null || :

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
