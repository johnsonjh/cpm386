# CP/M-386

<!-- Copyright (c) 2026 Jeffrey H. Johnson -->
<!-- SPDX-License-Identifier: MIT -->
<!-- scspell-id: 696e52ee-8276-11f1-b02c-80ee73e9b8e7 -->

**CP/M‑386** is CP/M for 386 protected mode, derived from CP/M‑68K.

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

|            System | Coverage |
|------------------:|:---------|
| CP/M‑68K&nbsp;1.2 | **100%** |
| CP/M&nbsp;2.2     | **98%**  |
| CP/M‑Plus         | **71%**  |
| DOS‑Plus          | **62%**  |
| MP/M&nbsp;2.1     | **50%**  |

* The **CP/M‑386** BDOS is at full parity with CP/M‑68K&nbsp;1.2.
* CP/M&nbsp;2.2 equivalence is lacking only
  [BDOS 27](https://www.seasip.info/Cpm/bdos.html#27) (which was deliberately
  omitted from CP/M‑68K by Digital Research).
* A large majority of the CP/M‑Plus (CP/M&nbsp;3) BDOS is also supported.
* More than 60% of the DOS‑Plus additions have been implemented.
* Approximately half of the MP/M extensions have been completed.  The missing
  functionality is largely the multi‑user, multi‑tasking, message queuing, and
  process control calls that don't apply to a single‑user CP/M implementation.

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

## QEMU Testing

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
                <th>50</th>
                <th>19133</th>
                <th>3533</th>
                <th>3147</th>
                <th>12453</th>
                <th>2661</th>
                <th>475526</th>
                <th>7194</th>
        </tr><tr>
                <th>C Header</th>
                <th>15</th>
                <th>1937</th>
                <th>319</th>
                <th>715</th>
                <th>903</th>
                <th>9</th>
                <th>74758</th>
                <th>1071</th>
        </tr><tr>
                <th>Makefile</th>
                <th>1</th>
                <th>769</th>
                <th>185</th>
                <th>117</th>
                <th>467</th>
                <th>48</th>
                <th>24976</th>
                <th>462</th>
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
                <th>206</th>
                <th>32</th>
                <th>0</th>
                <th>174</th>
                <th>0</th>
                <th>6257</th>
                <th>161</th>
        </tr></tbody>
        <tfoot><tr>
                <th>Total</th>
                <th>70</th>
                <th>22650</th>
                <th>4144</th>
                <th>4087</th>
                <th>14419</th>
                <th>2718</th>
                <th>595414</th>
                <th>9254</th>
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
