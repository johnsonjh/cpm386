# CP/M-386

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

## Compilation

Building **CP/M-386** is supported on **Solaris**, **illumos**, **FreeBSD**,
**OpenBSD**, **NetBSD**, **Haiku**, and **Linux**.

**GCC** build (default):

```sh
make && make test
```

**Clang** build:

```sh
make CC="clang" && make test
```

## QEMU Testing

[Multiboot](https://en.wikipedia.org/wiki/Multiboot_specification) kernel:

```sh
qemu-system-x86_64 -m 4G -serial stdio -monitor none -kernel "cpm386.elf"
```

Floppy ([MBR](https://en.wikipedia.org/wiki/Master_boot_record)) loader:

```sh
qemu-system-i386 -m 4G -serial stdio -monitor none -fda "floppy.img"
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
                <th>18672</th>
                <th>2698</th>
                <th>1509</th>
                <th>14465</th>
                <th>3161</th>
                <th>428436</th>
                <th>8151</th>
        </tr><tr>
                <th>C Header</th>
                <th>15</th>
                <th>1318</th>
                <th>137</th>
                <th>278</th>
                <th>903</th>
                <th>9</th>
                <th>54241</th>
                <th>1041</th>
        </tr><tr>
                <th>Assembly</th>
                <th>3</th>
                <th>574</th>
                <th>66</th>
                <th>91</th>
                <th>417</th>
                <th>0</th>
                <th>12945</th>
                <th>396</th>
        </tr><tr>
                <th>Makefile</th>
                <th>1</th>
                <th>454</th>
                <th>84</th>
                <th>8</th>
                <th>362</th>
                <th>39</th>
                <th>14928</th>
                <th>362</th>
        </tr><tr>
                <th>Markdown</th>
                <th>1</th>
                <th>162</th>
                <th>26</th>
                <th>0</th>
                <th>136</th>
                <th>0</th>
                <th>4654</th>
                <th>123</th>
        </tr></tbody>
        <tfoot><tr>
                <th>Total</th>
                <th>67</th>
                <th>21180</th>
                <th>3011</th>
                <th>1886</th>
                <th>16283</th>
                <th>3209</th>
                <th>515204</th>
                <th>10044</th>
        </tr></tfoot></table>
<!-- scc-end -->

## License

* **CP/M-386** is distributed under the terms of the [MIT License](LICENSE).

* In 2022, DRDOS, Inc. explicitly granted unlimited authorization to use,
  distribute, modify, enhance, and otherwise make available CP/M and
  its derivatives.
