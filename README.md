# CP/M-386

**CP/M-386** is CP/M for 386 protected mode, derived from CP/M-68K.

## Overview

**CP/M-386 is currently in the** ***very*** **early development stages.**

* Full 32-bit protected mode implementation with Ring-3 TPA.
* Bootable via 1.44MB floppy MBR or GRUB Multiboot kernel
* Supports VGA text (`0xB8000`) and/or COM1 serial (9600/N/8/1, `0x3F8`) consoles.
* No floppy/hard disk/CD/USB/network/sound/other drivers (yet).

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

## Testing

```sh
qemu-system-x86_64 -m 4G -serial stdio -monitor none -kernel "cpm386.elf"
```

```sh
qemu-system-i386 -m 4G -serial stdio -monitor none -fda "floppy.img"
```

## License

* [**MIT License**](LICENSE)
