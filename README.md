# CP/M-386

<!-- Copyright (c) 2026 Jeffrey H. Johnson -->
<!-- SPDX-License-Identifier: MIT -->
<!-- scspell-id: 696e52ee-8276-11f1-b02c-80ee73e9b8e7 -->

**CP/M‑386** is CP/M for 386 protected mode, derived from CP/M‑68K.

---

<!-- toc -->

- [Overview](#overview)
- [CP/M compatibility](#cpm-compatibility)
- [Screenshots](#screenshots)
- [Build requirements](#build-requirements)
- [Compilation](#compilation)
- [QEMU testing](#qemu-testing)
  * [QEMU notes](#qemu-notes)
- [Hardware support](#hardware-support)
- [Included utilities](#included-utilities)
- [Future](#future)
- [Code statistics](#code-statistics)
- [License](#license)

<!-- tocstop -->

---

## Overview

**CP/M‑386 is currently in the** ***very*** **early development stages.**

* Full 32‑bit [protected mode](https://en.wikipedia.org/wiki/Protected_mode)
  implementation with
  [Ring‑3 TPA](https://en.wikipedia.org/wiki/Protection_ring).
* Bootable via 3.5" 1.44MB floppy disk
  [MBR](https://en.wikipedia.org/wiki/Master_boot_record) or GRUB
  [Multiboot](https://en.wikipedia.org/wiki/Multiboot_specification) kernel.
* Supports [VGA text](https://en.wikipedia.org/wiki/VGA_text_mode) (`0xB8000`)
  and/or [COM1 serial](https://en.wikipedia.org/wiki/Serial_port)
  (9600/N/8/1, `0x3F8`) consoles.
* **No floppy/hard disk/CD/USB/network/sound/other drivers** (**yet**).

## CP/M compatibility

|            System | BDOS coverage |
|------------------:|:--------------|
| CP/M‑68K&nbsp;1.2 | **100%**      |
| CP/M&nbsp;2.2     | **98%**       |
| CP/M‑Plus         | **71%**       |
| DOS‑Plus          | **62%**       |
| MP/M&nbsp;2.1     | **50%**       |

* The **CP/M‑386** BDOS is at full parity with CP/M‑68K&nbsp;1.2.
* CP/M&nbsp;2.2 equivalence is lacking only
  [BDOS 27](https://www.seasip.info/Cpm/bdos.html#27) (which was deliberately
  omitted from CP/M‑68K by Digital Research).
* A large majority of the CP/M‑Plus (CP/M&nbsp;3) BDOS is also supported.
* More than 60% of the DOS‑Plus additions have been implemented.
* Approximately half of the MP/M extensions have been completed.
  * The missing functionality is largely the multi‑user, multi‑tasking,
    message queuing, and process control calls that don't apply to a
    single‑user CP/M implementation.
* Unique **CP/M‑386**‑specific BDOS extensions have been added to accommodate
  new features like direct video access and high‑resolution timing.

## Screenshots

<div style="display:flex; justify-content:center; align-items:center;">
 <a href=".img/VGA.png" style="flex:1; text-align:center;">
  <img src=".img/VGA.png" style="width:100%;">
 </a>
 <a href=".img/SER.png" style="flex:1; text-align:center;">
  <img src=".img/SER.png" style="width:100%;">
 </a>
</div>

## Build requirements

* [AWK](https://en.wikipedia.org/wiki/AWK)
* [Cpmtools](https://www.moria.de/~michael/cpmtools/)
* [GNU Binutils](https://www.gnu.org/software/binutils/)
* [GNU Coreutils](https://www.gnu.org/software/coreutils/)
* [GNU GCC](https://gcc.gnu.org/) or [LLVM Clang](https://clang.llvm.org)
* [GNU Make](https://www.gnu.org/software/make/)
* [NASM](https://nasm.us/)

[QEMU](https://www.qemu.org) is recommended for testing.

## Compilation

Building **CP/M-386** is supported on **Linux**, **NetBSD**, **OpenBSD**, and
and **Haiku**:

**GCC** build (default):

```sh
make -j "$(nproc 2> /dev/null || printf '%s' 1)"
make test
```

**Clang** build:

```sh
make -j "$(nproc 2> /dev/null || printf '%s' 1)" CC="clang"
make test
```

## QEMU testing

[Multiboot](https://en.wikipedia.org/wiki/Multiboot_specification)
kernel (recommended):

```sh
qemu-system-i386 -m 1G -serial stdio -monitor none -kernel "cpm386.elf"
```

Floppy [MBR](https://en.wikipedia.org/wiki/Master_boot_record) loader:

```sh
qemu-system-i386 -m 1G -serial stdio -monitor none -fda "floppy.img"
```

### QEMU notes

* Use `-display none` to disable VGA video (and use *only* serial console).
* Use `-serial none` to disable the serial UART (and use *only* VGA console).

## Hardware support

* Compatible with 386 (and later systems) with 1MB (or more) memory.
* Systems using either PC BIOS or UEFI (with CSM) are supported.
* VGA, 8042 PS/2, 8250/16450/16550 UART, and 8253/8254 PIT are supported.

## Included utilities

|        Program | Description                                                                                        |
|---------------:|:---------------------------------------------------------------------------------------------------|
| `ACLOCKDV.386` | [aclock](https://github.com/tenox7/aclock) (VGA console version)                                   |
| `ACLOCKVT.386` | [aclock](https://github.com/tenox7/aclock) (ANSI terminal version)                                 |
| `BIG.386`      | Multi‑extent loading test executable                                                               |
| `CLS.386`      | Clear screen (BDOS 221)                                                                            |
| `DELAY.386`    | Delay test ([BDOS 141](https://www.seasip.info/Cpm/bdos.html#141))                                 |
| `DEMO.SUB`     | SUBMIT demonstration                                                                               |
| `DUMPDIR.386`  | Directory entry dump utility ([BDOS 17](https://www.seasip.info/Cpm/bdos.html#17), [BDOS 18](https://www.seasip.info/Cpm/bdos.html#18)) |
| `DUMPFCB.386`  | File control block dump utility ([BDOS 15](https://www.seasip.info/Cpm/bdos.html#15))              |
| `ENV.DAT`      | Environment data file                                                                              |
| `FPARSE.386`   | F_PARSE test ([BDOS 152](https://www.seasip.info/Cpm/bdos.html#152))                               |
| `GETSN.386`    | Display serial number ([BDOS 107](https://www.seasip.info/Cpm/bdos.html#107))                      |
| `HD.386`       | Hex dump utility                                                                                   |
| `HELLO.386`    | Hello world! (the first CP/M‑386 program!)                                                         |
| `ILLEGAL.386`  | Ring‑3 protection and exception handler test                                                       |
| `IOTEST.386`   | File I/O BDOS tests                                                                                |
| `LRBC.386`     | Query Last Record Byte Count                                                                       |
| `LS.386`       | List files (with sizes)                                                                            |
| `MEM.386`      | Memory map utility (BDOS 227)                                                                      |
| `MORE.386`     | UNIX `more`‑style pager                                                                            |
| `OD.386`       | Octal dump utility                                                                                 |
| `PAUSE.386`    | Wait for keypress                                                                                  |
| `PRINTENV.386` | Print environment and system data                                                                  |
| `PROFILE.SUB`  | SUBMIT script (automatically executed at boot)                                                     |
| `RC.386`       | Return code test and query ([BDOS 108](https://www.seasip.info/Cpm/bdos.html#108))                 |
| `README.TXT`   | Sample text file                                                                                   |
| `REBOOT.386`   | Reboot utility (BDOS 220)                                                                          |
| `RM.386`       | UNIX `rm`‑like interactive file deletion utility                                                   |
| `SEROFF.386`   | Disable serial console (BDOS 223)                                                                  |
| `SERON.386`    | Enable serial console (BDOS 223)                                                                   |
| `STAT.386`     | Digital Research STAT (A port of Zilog CP/M‑Z8000 STAT v1.0C 01/03/84)                             |
| `SYNC.386`     | Synchronize disks ([BDOS 48](https://www.seasip.info/Cpm/bdos.html#48))                            |
| `TEST211.386`  | Numeric format test ([BDOS 211](https://www.seasip.info/Cpm/bdos.html#211))                        |
| `TICKS.386`    | High‑resolution timer tests (BDOS 225, BDOS 226)                                                   |
| `TOD.386`      | Get (and set) Time of Day clock ([BDOS 104](https://www.seasip.info/Cpm/bdos.html#104), [BDOS 105](https://www.seasip.info/Cpm/bdos.html#105)) |
| `TOUCH.386`    | Create an empty file                                                                               |
| `TRUNC.386`    | Truncation tests ([BDOS 99](https://www.seasip.info/Cpm/bdos.html#99))                             |
| `TSEC.386`     | Get date and time ([BDOS 155](https://www.seasip.info/Cpm/bdos.html#155))                          |
| `VER.386`      | Display OS version ([BDOS 163](https://www.seasip.info/Cpm/bdos.html#163))                         |
| `VGAOFF.386`   | Disable VGA console (BDOS 222)                                                                     |
| `VGAON.386`    | Enable VGA console (BDOS 222)                                                                      |
| `VGATEXT.386`  | VGA direct access demo (BDOS 224)                                                                  |

## Future

I hope to eventually implement the following features in **CP/M‑386**.  Treat
this section as *wishlist* and not a *roadmap*!

* OS enhancements:
  * Finish remaining unimplemented BDOS functions
  * Better keyboard support (Alt+nnn‑style 8-bit input, numeric keypad,
    function keys, arrow/motion keys, etc.)
  * User‑defined configurable first‑class RAM disk (default to M:)
  * Drivers for floppy disk, hard disk, and parallel printer
  * Disk caching
  * Drivers for networking (with CP/Net NDOS and generic packet interface)
  * DOS‑PLUS (enhanced VT52) compatible VGA console (with optional ADM3A mode)
  * Enhance filesystem with CP/M‑Plus parity (timestamps, passwords, etc.)
  * Extend user area support from 16 to 32 user areas per disk
  * Extend direct video support for mode switching and framebuffer
    (Doom port!)
  * Support for non-PIT high‑resolution timers (HPET, APIC, TSC, RDTSC)
  * ZCPR TCAP‑like standard terminal capability database
  * DOS‑PLUS / CP/M‑Plus style status line with user customization
  * GSX graphics (mostly standard GSX‑stack of GDD, GIOS, GDOS, etc.)

* Transparent emulation:
  * Transparent CP/M‑80 Z80 emulation for CP/M‑80 `COM` programs
  * Transparent CP/M‑86 execution environment for CP/M-86 `CMD` programs
    (utilizing V86 mode to avoid emulation overhead)
  * Transparent Heath [HDOS](https://en.wikipedia.org/wiki/HDOS) 8080
    emulation for HDOS `ABS` programs
    ([HRUN](https://heathkit.garlanger.com/software/library/HUG/docs/HRUN_doc.pdf)
    is prior art)

* CCP enhancements:
  * Move the CCP out of Ring 0 to Ring 2
  * Stackable execution of commands
  * Command timing
  * ZCPR ECP‑like search paths/named directories/aliases
  * SET/UNSET commands to manage environment (`ENVIRON.DAT`) variables
  * Command editing and recall (terminal and VGA console aware)
  * ALIAS/UNALIAS internal commands
  * Enhance SUBMIT:
    * Allow nested SUBMIT scripts
    * Allow suppressing SUBMIT command echoing (*i.e.*, `@` and `ECHO ON`/`OFF`)
    * Allow early exit from SUBMIT scripts depending on return codes
    * Refactor CCP to eliminate the `dirflag` global
    * Eliminate SUBMIT temporary file usage and run completely from memory
    * ZCPR+SUPERSUB‑style conditional control (`IF`/`ELSE`/`FI`, etc.)
    * SUPERSUB‑style interactive user input for SUBMIT scripts

* Software/ports needed:
  * Digital Research ED (port Zilog CP/M‑Z8000 ED 08/1982)
  * Digital Research PIP (port Zilog CP/M‑Z8000 PIP 1.0A 01/03/1984)
  * Digital Research HELP clone (VMS‑style HELP tool)
  * Memory test utility (writes and verifies the full TPA)
  * less‑style pager
  * [6FORTH](https://github.com/johnsonjh/6FORTH) port
  * [`pc`](https://github.com/johnsonjh/pc) port
  * UNIX `bc` port
  * UNIX‑style `cp` copy command that supports user areas, LRBC, etc.
  * Compression tools (`lha`, `gzip`, `compress`, `zip`, etc.) w/LRBC support
  * [TPZASM](https://github.com/johnsonjh/tpzasm) port
  * [T3X/0](https://www.t3x.org/t3x/0/index_d.html)
  * Kermit and YAM ports (requires a proper IOBYTE implementation!)
  * Nice hex editor (supporting direct modification of the TPA)
  * Editors! MINCE, uEmacs, CALVIN, `g`, `s`, `te`,
    [VEDIT-PLUS](https://github.com/johnsonjh/VEDIT)?
  * [Ack](https://github.com/davidgiven/ack) CP/M‑386 target
  * More tools from
    [tsupplis](https://github.com/tsupplis/cpm86-hacking)
  * A native DDT‑like debugger
  * A real assembler (most likely [NASM](https://nasm.us/))
  * A real C standard library customized for CP/M‑386
    (probably [newlib](https://sourceware.org/git/newlib-cygwin.git))
  * A real CP/M‑386 native C compiler

## Code statistics

<!-- scc-start -->
<table id="scc-table">
        <thead><tr>
                <th>Language</th>
                <th>Files</th>
                <th>Lines</th>
                <th>Blank</th>
                <th>Comment</th>
                <th>Code</th>
                <th>Complexity</th>
                <th>Bytes</th>
                <th>Uloc</th>
        </tr></thead>
        <tbody><tr>
                <th>C</th>
                <th>51</th>
                <th>20941</th>
                <th>3915</th>
                <th>3319</th>
                <th>13707</th>
                <th>2839</th>
                <th>511204</th>
                <th>7861</th>
        </tr><tr>
                <th>C Header</th>
                <th>15</th>
                <th>1941</th>
                <th>319</th>
                <th>715</th>
                <th>907</th>
                <th>9</th>
                <th>74836</th>
                <th>1074</th>
        </tr><tr>
                <th>Makefile</th>
                <th>1</th>
                <th>788</th>
                <th>191</th>
                <th>120</th>
                <th>477</th>
                <th>48</th>
                <th>25592</th>
                <th>472</th>
        </tr><tr>
                <th>Assembly</th>
                <th>3</th>
                <th>605</th>
                <th>75</th>
                <th>108</th>
                <th>422</th>
                <th>0</th>
                <th>13897</th>
                <th>409</th>
        </tr><tr>
                <th>Markdown</th>
                <th>1</th>
                <th>357</th>
                <th>47</th>
                <th>0</th>
                <th>310</th>
                <th>0</th>
                <th>16098</th>
                <th>296</th>
        </tr></tbody>
        <tfoot><tr>
                <th>Total</th>
                <th>71</th>
                <th>24632</th>
                <th>4547</th>
                <th>4262</th>
                <th>15823</th>
                <th>2896</th>
                <th>641627</th>
                <th>10069</th>
        </tr></tfoot></table>
<!-- scc-end -->

## License

* **CP/M‑386** is distributed under the terms of the permissive
  [MIT&nbsp;License](LICENSE).

* Bryan W. Sparks of DRDOS,&nbsp;Inc. dba DeviceLogics&nbsp;LLC, successor
  in interest to Digital&nbsp;Research,&nbsp;Inc.’s CP/M assets, explicitly
  grants an unlimited authorization to use, distribute, modify, enhance, and
  otherwise make available CP/M technology, including the CP/M operating
  systems and their derivatives.

<!--
Local Variables:
mode: markdown
indent-tabs-mode: nil
fill-column: 80
eval: (setq-local display-fill-column-indicator-column 72)
eval: (display-fill-column-indicator-mode 1)
End:
-->
<!-- vim: set ft=markdown expandtab cc=80 : -->
<!-- EOF -->
