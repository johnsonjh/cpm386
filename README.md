# CP/M-386

**CP/M-386** is CP/M for 386 protected mode, derived from CP/M-68K.

## Overview

**CP/M-386 is currently in the** ***very*** **early development stages.**

* Full 32-bit [protected mode](https://en.wikipedia.org/wiki/Protected_mode)
  implementation with
  [Ring-3 TPA](https://en.wikipedia.org/wiki/Protection_ring).
* Bootable via 1.44MB floppy
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
* [cpmtools](https://www.moria.de/~michael/cpmtools/)
* [GCC](https://gcc.gnu.org/) (supporting Intel x86 `-m32`)
* [GNU binutils](https://www.gnu.org/software/binutils/)
* [GNU coreutils](https://www.gnu.org/software/coreutils/)
* [GNU make](https://www.gnu.org/software/make/)
* [NASM](https://nasm.us/)

### Common distributions

#### RHEL

1. Enable the [EPEL](https://docs.fedoraproject.org/en-US/epel/) repositories.
2. Install the appropriate `libdsk` and `cpmtools` packages from
   [Koji](https://koji.fedoraproject.org/).
3. Install the required build dependencies:

```sh
dnf install binutils coreutils gawk gcc make nasm
```

#### Fedora

```sh
dnf install binutils coreutils cpmtools gawk gcc make nasm
```

#### Debian

```sh
apt install binutils coreutils cpmtools gawk gcc make nasm
```

#### Alpine

Alpine users will need to manually install
[cpmtools](https://www.moria.de/~michael/cpmtools/).

```sh
apk add binutils coreutils gawk gcc make nasm
```

## Building

* `make && make test`

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
                <th>18653</th>
                <th>2681</th>
                <th>1509</th>
                <th>14463</th>
                <th>3161</th>
                <th>428390</th>
                <th>8149</th>
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
                <th>378</th>
                <th>72</th>
                <th>17</th>
                <th>289</th>
                <th>21</th>
                <th>13031</th>
                <th>299</th>
        </tr><tr>
                <th>Markdown</th>
                <th>1</th>
                <th>179</th>
                <th>32</th>
                <th>0</th>
                <th>147</th>
                <th>0</th>
                <th>4857</th>
                <th>131</th>
        </tr></tbody>
        <tfoot><tr>
                <th>Total</th>
                <th>67</th>
                <th>21102</th>
                <th>2988</th>
                <th>1895</th>
                <th>16219</th>
                <th>3191</th>
                <th>513464</th>
                <th>9987</th>
        </tr></tfoot></table>
<!-- scc-end -->

## License

* [MIT License](LICENSE)
