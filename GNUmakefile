################################################################################
# CP/M-386 - GNUmakefile
# Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
# SPDX-License-Identifier: MIT
# scspell-id: b1bb2880-826f-11f1-a5c2-80ee73e9b8e7
################################################################################

################################################################################

export LC_ALL=C
export MAKE=$(shell printf '%s'\
	"$${MAKE:-$$(command -v gmake 2>&1 || command -v make 2>&1)}")

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
	-Wno-unused-parameter \
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
BIG_IMG_SIZE:=71680

################################################################################

# Truncate initial ramdisk image to 384K (messy but OK temporary for bringup)
# maximum size that can be set is 448 (for now!)
RAMDISK_KB:=384

################################################################################

CPMFS:=4mb-hd

################################################################################

.PHONY: all

all: $(TARGET) stage1.bin stage2.bin os.bin floppy.img
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
	    exit 1; fi
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

SHOLE = shole
$(SHOLE): shole.c
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
	  exit 1; fi
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
	  exit 1; fi
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
	  exit 1; fi
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
	  exit 1; fi
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

vgatext.386: vgatext.bin $(MK386)
	./$(MK386) ./vgatext.bin ./vgatext.386 0x100
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

big.bin: big.c user.ld
	$(CC) $(CFLAGS) -c -o ./big.o ./big.c
	$(CC) $(LDEXTRA) $(LTO_FLAGS) -Wl,--build-id=none -nostdlib \
		-Wl,-m,$(ELF_I386) -no-pie -Wl,-T,./user.ld -o ./big.elf ./big.o
	@entry=$$($(NM) ./big.elf | $(AWK) '$$NF=="_start"{print $$1}'); \
	  if [ "$$entry" != "00000100" ]; then \
	  tput bold 2> /dev/null || :; tput setaf 1 2> /dev/null || :; \
	  $(PRINTF) '%s\n' "ERROR: big _start"; \
	  tput sgr0 2> /dev/null || :; \
	  exit 1; fi
	$(OBJCOPY) -O binary ./big.elf ./big_stub.bin
	@stub=$$(wc -c < ./big_stub.bin); \
	  if [ "$$stub" -ge $(BIG_IMG_SIZE) ]; then \
	    tput bold 2> /dev/null || :; tput setaf 1 2> /dev/null || :; \
	    $(PRINTF) '%s\n' "ERROR: big stub $$stub >= $(BIG_IMG_SIZE)"; \
	    tput sgr0 2> /dev/null || :; \
	    exit 1; fi; \
	  pad=$$(( $(BIG_IMG_SIZE) - stub )); \
	  $(CP) -f ./big_stub.bin ./big.bin; \
	  $(DD) if="/dev/zero" bs="1" count="$$pad" | \
	    env LC_ALL=C $(TR) '\0' '\220' >> ./big.bin
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

big.386: big.bin $(MK386)
	./$(MK386) ./big.bin ./big.386 0x100
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

ramdisk.bin: \
			$(SHOLE) \
			aclockdv.386 \
			aclockvt.386 \
			big.386 \
			cleartpa.386 \
			cls.386 \
			delay.386 \
			dumpdir.386 \
			dumpfcb.386 \
			fparse.386 \
			getsn.386 \
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
			od.386 \
			pause.386 \
			printenv.386 \
			rc.386 \
			reboot.386 \
			rm.386 \
			seroff.386 \
			seron.386 \
			stat.386 \
			sync.386 \
			test211.386 \
			ticks.386 \
			tod.386 \
			touch.386 \
			truncate.386 \
			trunctst.386 \
			tsec.386 \
			ver.386 \
			vgaoff.386 \
			vgaon.386 \
			vgatext.386
	$(RM) -f /tmp/ramdisk.tmp
	$(RM) -rf /tmp/cpmd
	mkdir -p /tmp/cpmd
	$(PRINTF) 'This is README.TXT from the CP/M-386 RAM disk.\r\n\x1a' \
		> /tmp/cpmd/README.TXT
	$(PRINTF) '; Sample ENV.DAT for CP/M-386\r\nHELLO=World\r\n\x1a' \
		> /tmp/cpmd/ENV.DAT
	$(PRINTF) '; DEMO.SUB - SUBMIT on CP/M-386 testing\r\n; NOTE: No nested SUBMIT (yet!)\r\nSEROFF\r\nSERON\r\nVGAOFF\r\nVGAON\r\nVER\r\nGETSN\r\nMEM\r\nTOD\r\nTSEC\r\nTICKS\r\nTOD\r\nTSEC\r\nTICKS\r\nTOD\r\nTSEC\r\nTICKS\r\nDELAY\r\nSTAT DSK:\r\nSTAT STAT.386 SIZE\r\nPRINTENV\r\nTEST211\r\nLS -A\r\nDIR\r\nLS -L BIG.*\r\nBIG\r\nLRBC BIG.386\r\nTRUNCTST\r\nIOTEST\r\nSYNC\r\nFPARSE\r\nILLEGAL\r\nLRBC README.TXT\r\nTOUCH NEW.DAT\r\nERA NEW.DAT\r\nDUMPFCB DEMO.SUB\r\nDUMPDIR DEMO.*\r\nHD CLS.386\r\nOD CLS.386\r\nERA TRUNC.DAT\r\nRC 1\r\nREN IOWORK.D4T=IOWORK.DAT\r\nRM IOWORK.D4T\r\nRC\r\nHELLO\r\n; Run again from program still existing in TPA\r\nGO\r\n; End of DEMO.SUB\r\n\x1a' \
		> /tmp/cpmd/DEMO.SUB
	$(PRINTF) 'VER\r\n' > /tmp/cpmd/PROFILE.SUB
	$(CP) -f ./aclockdv.386 /tmp/cpmd/ACLOCKDV.386
	$(CP) -f ./aclockvt.386 /tmp/cpmd/ACLOCKVT.386
	$(CP) -f ./big.386 /tmp/cpmd/BIG.386
	$(CP) -f ./cls.386 /tmp/cpmd/CLS.386
	$(CP) -f ./delay.386 /tmp/cpmd/DELAY.386
	$(CP) -f ./dumpdir.386 /tmp/cpmd/DUMPDIR.386
	$(CP) -f ./dumpfcb.386 /tmp/cpmd/DUMPFCB.386
	$(CP) -f ./fparse.386 /tmp/cpmd/FPARSE.386
	$(CP) -f ./getsn.386 /tmp/cpmd/GETSN.386
	$(CP) -f ./hd.386 /tmp/cpmd/HD.386
	$(CP) -f ./hello.386 /tmp/cpmd/HELLO.386
	$(CP) -f ./illegal.386 /tmp/cpmd/ILLEGAL.386
	$(CP) -f ./iotest.386 /tmp/cpmd/IOTEST.386
	$(CP) -f ./julia.386 /tmp/cpmd/JULIA.386
	$(CP) -f ./lrbc.386 /tmp/cpmd/LRBC.386
	$(CP) -f ./ls.386 /tmp/cpmd/LS.386
	$(CP) -f ./mandel.386 /tmp/cpmd/MANDEL.386
	$(CP) -f ./mem.386 /tmp/cpmd/MEM.386
	$(CP) -f ./more.386 /tmp/cpmd/MORE.386
	$(CP) -f ./od.386 /tmp/cpmd/OD.386
	$(CP) -f ./pause.386 /tmp/cpmd/PAUSE.386
	$(CP) -f ./cleartpa.386 /tmp/cpmd/CLEARTPA.386
	$(CP) -f ./printenv.386 /tmp/cpmd/PRINTENV.386
	$(CP) -f ./rc.386 /tmp/cpmd/RC.386
	$(CP) -f ./reboot.386 /tmp/cpmd/REBOOT.386
	$(CP) -f ./rm.386 /tmp/cpmd/RM.386
	$(CP) -f ./seroff.386 /tmp/cpmd/SEROFF.386
	$(CP) -f ./seron.386 /tmp/cpmd/SERON.386
	$(CP) -f ./stat.386 /tmp/cpmd/STAT.386
	$(CP) -f ./sync.386 /tmp/cpmd/SYNC.386
	$(CP) -f ./test211.386 /tmp/cpmd/TEST211.386
	$(CP) -f ./ticks.386 /tmp/cpmd/TICKS.386
	$(CP) -f ./tod.386 /tmp/cpmd/TOD.386
	$(CP) -f ./touch.386 /tmp/cpmd/TOUCH.386
	$(CP) -f ./truncate.386 /tmp/cpmd/TRUNCATE.386
	$(CP) -f ./trunctst.386 /tmp/cpmd/TRUNCTST.386
	$(CP) -f ./tsec.386 /tmp/cpmd/TSEC.386
	$(CP) -f ./ver.386 /tmp/cpmd/VER.386
	$(CP) -f ./vgaoff.386 /tmp/cpmd/VGAOFF.386
	$(CP) -f ./vgaon.386 /tmp/cpmd/VGAON.386
	$(CP) -f ./vgatext.386 /tmp/cpmd/VGATEXT.386
	mkfs.cpm -f $(CPMFS) /tmp/ramdisk.tmp
	cpmcp -f $(CPMFS) /tmp/ramdisk.tmp \
	  /tmp/cpmd/ACLOCKDV.386 \
	  /tmp/cpmd/ACLOCKVT.386 \
	  /tmp/cpmd/BIG.386 \
	  /tmp/cpmd/CLS.386 \
	  /tmp/cpmd/CLEARTPA.386 \
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
	  /tmp/cpmd/JULIA.386 \
	  /tmp/cpmd/LRBC.386 \
	  /tmp/cpmd/LS.386 \
	  /tmp/cpmd/MANDEL.386 \
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
	  /tmp/cpmd/TRUNCATE.386 \
	  /tmp/cpmd/TRUNCTST.386 \
	  /tmp/cpmd/TSEC.386 \
	  /tmp/cpmd/VER.386 \
	  /tmp/cpmd/VGAOFF.386 \
	  /tmp/cpmd/VGAON.386 \
	  /tmp/cpmd/VGATEXT.386 \
	  0:
	./$(SHOLE) -w /tmp/ramdisk.tmp BIG.386
	@RDS=$$($(PRINTF) '%d' "$$($(OD) -A x -t x2 /tmp/ramdisk.tmp | \
		$(GREP) -v '^*$$' | tail -2 | head -1 | \
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
	$(DD) if="/dev/zero" of="./ramdisk.bin" bs="1024" count="$(RAMDISK_KB)"
	$(DD) if="/tmp/ramdisk.tmp" of="./ramdisk.bin" conv="notrunc"
	$(RM) -rf /tmp/ramdisk.tmp /tmp/cpmd
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

bringup.o: bringup.c ramdisk.bin bringup.h
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

bios.o: bios.c
	$(CC) $(CFLAGS) -c -o ./$@ ./$<
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

rtc.o: rtc.c rtc.h
	$(CC) $(CFLAGS) -c -o ./$@ ./$<
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

pit.o: pit.c pit.h
	$(CC) $(CFLAGS) -c -o ./$@ ./$<
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

pmode.o: pmode.c pmode.h
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

mbentry.o: mbentry.s mltiboot.h
	$(NASM) $(NASMFLAGS) $(NASMDEBUG) -I. -l ./${@:.o=.lst} -o ./$@ ./$<
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

bss.inc: $(TARGET)
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
	ke=$$($(NM) $(TARGET) | \
		$(AWK) '/__kernel_end/ { print $$1 }'); \
			bytes=$$((0x$$ke - 0x10000)); \
			sec=$$(( (bytes + 511) / 512 + 2 )); \
			$(PRINTF) '%s\n' "SECTORS_TO_LOAD equ $$sec" >> ./bss.inc
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

stage2.bin: stage2.s layout.inc bss.inc
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

$(TARGET): $(OBJS) linker.ld
	$(CC) $(LDEXTRA) $(LDFLAGS) -o ./$@ ./$(OBJS)
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

floppy.img: stage1.bin stage2.bin os.bin
	cat ./stage1.bin ./stage2.bin ./os.bin > ./payload.bin
	$(PRINTF) '\x1a' >> ./payload.bin
	$(DD) if="/dev/zero" of="$@" bs="1024" count="1440"
	$(DD) if="./payload.bin" of="$@" conv="notrunc"
	@set -- "$$(wc -c < ./payload.bin)"; pbytes="$$1"; \
	  if [ "$$pbytes" -gt 1474560 ]; then \
	    tput bold 2> /dev/null || :; tput setaf 1 2> /dev/null || :; \
	    $(PRINTF) '%s\n' "ERROR: payload.bin $$pbytes >= 1474560 maximum"; \
	    tput sgr0 2> /dev/null || :; \
	    exit 1; fi
	@tput setaf 6 2> /dev/null || :
	@$(PRINTF) '*** floppy.img usage: ~%d KiB\n' \
		$$(("$$($(OD) -A x -t x2 ./floppy.img | $(GREP) -v '^*$$' | \
		tail -2 | head -1 | $(AWK) '{ print "0x"a$$1 }')" / 1024)) || :
	@tput sgr0 2> /dev/null || :
	@tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "$@ built successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

.PHONY: clean distclean

clean distclean:
	$(RM) -f ./*.o ./*.elf ./*.img /*.log ./*.bin ./*.386 ./*.lst \
		./bss.inc ./testbdos ./*.su ./*.ci ./$(MK386) ./$(SHOLE) ./$(TARGET)
	@ccache -cC > /dev/null 2>&1 || :
	@tput bold 2> /dev/null || :; tput setaf 2 2> /dev/null || :
	@$(PRINTF) '%s\r\n' "Clean completed successfully."
	@tput sgr0 2> /dev/null || :

################################################################################

testbdos: testbdos.c bdosmain.o bdosmisc.o bdosrw.o conbdos.o fileio.o \
	dskutil.o iosys.o ccp.o bringup.o
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
