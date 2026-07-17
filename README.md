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
* **No floppy/hard disk/CD/USB/network/sound/other drivers** (***yet***).

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
* [GCC](https://gcc.gnu.org/) (supporting `-m32`)
* [GNU binutils](https://www.gnu.org/software/binutils/)
* [GNU coreutils](https://www.gnu.org/software/coreutils/)
* [GNU make](https://www.gnu.org/software/make/)
* [NASM](https://nasm.us/)

## Building

* `make && make test`

## QEMU Testing

* [Multiboot](https://en.wikipedia.org/wiki/Multiboot_specification) kernel:

  ```sh
  qemu-system-x86_64 -m 4G -serial stdio -monitor none -kernel "cpm386.elf"
  ```

* Floppy (MBR) loader:

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
                <th>41</th>
                <th>14486</th>
                <th>1980</th>
                <th>1341</th>
                <th>11165</th>
                <th>2507</th>
                <th>357474</th>
                <th>6579</th>
        </tr><tr>
                <th>C Header</th>
                <th>11</th>
                <th>1014</th>
                <th>100</th>
                <th>245</th>
                <th>669</th>
                <th>9</th>
                <th>38135</th>
                <th>819</th>
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
                <th>343</th>
                <th>64</th>
                <th>16</th>
                <th>263</th>
                <th>20</th>
                <th>11844</th>
                <th>274</th>
        </tr><tr>
                <th>Markdown</th>
                <th>1</th>
                <th>145</th>
                <th>21</th>
                <th>0</th>
                <th>124</th>
                <th>0</th>
                <th>4156</th>
                <th>116</th>
        </tr></tbody>
        <tfoot><tr>
                <th>Total</th>
                <th>57</th>
                <th>16562</th>
                <th>2231</th>
                <th>1693</th>
                <th>12638</th>
                <th>2536</th>
                <th>424554</th>
                <th>8159</th>
        </tr></tfoot></table>
<!-- scc-end -->

## License

* [MIT License](LICENSE)
