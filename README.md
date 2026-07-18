# CP/M-386

<!-- Copyright (c) 2026 Jeffrey H. Johnson -->
<!-- SPDX-License-Identifier: MIT -->
<!-- scspell-id: 696e52ee-8276-11f1-b02c-80ee73e9b8e7 -->

**CP/M-386** is CP/M for 386 protected mode, derived from CP/M-68K.

## Overview

**CP/M-386 is currently in the** ***very*** **early development stages.**

* Full 32-bit [protected mode](https://en.wikipedia.org/wiki/Protected_mode)
  implementation with
  [Ring-3 TPA](https://en.wikipedia.org/wiki/Protection_ring).
* Bootable via 3.5" 1.44MB floppy disk
  [MBR](https://en.wikipedia.org/wiki/Master_boot_record) or GRUB
  [Multiboot](https://en.wikipedia.org/wiki/Multiboot_specification) kernel.
* Supports [VGA text](https://en.wikipedia.org/wiki/VGA_text_mode) (`0xB8000`)
  and/or [COM1 serial](https://en.wikipedia.org/wiki/Serial_port)
  (9600/N/8/1, `0x3F8`) consoles.
* **No floppy/hard disk/CD/USB/network/sound/other drivers** (**yet**).

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
                <th>47</th>
                <th>18750</th>
                <th>3452</th>
                <th>3049</th>
                <th>12249</th>
                <th>2641</th>
                <th>467100</th>
                <th>7119</th>
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
                <th>747</th>
                <th>179</th>
                <th>114</th>
                <th>454</th>
                <th>48</th>
                <th>24247</th>
                <th>449</th>
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
                <th>186</th>
                <th>29</th>
                <th>0</th>
                <th>157</th>
                <th>0</th>
                <th>5358</th>
                <th>144</th>
        </tr></tbody>
        <tfoot><tr>
                <th>Total</th>
                <th>67</th>
                <th>22225</th>
                <th>4054</th>
                <th>3986</th>
                <th>14185</th>
                <th>2698</th>
                <th>585360</th>
                <th>9149</th>
        </tr></tfoot></table>
<!-- scc-end -->

## License

* **CP/M-386** is distributed under the terms of the permissive
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
