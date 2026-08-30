# CP/M-386

<!-- CP/M-386 - README.md -->
<!-- Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com> -->
<!-- SPDX-License-Identifier: MIT -->
<!-- scspell-id: 696e52ee-8276-11f1-b02c-80ee73e9b8e7 -->

**CP/M‑386** is **CP/M** for 386 protected mode, derived from **CP/M‑68K**.

---

<!-- toc -->

- [Overview](#overview)
- [Hardware support](#hardware-support)
- [CP/M compatibility](#cpm-compatibility)
- [Build requirements](#build-requirements)
- [Optional dependencies](#optional-dependencies)
- [Downloads](#downloads)
- [Compilation](#compilation)
- [Docker build](#docker-build)
- [Build output](#build-output)
- [QEMU testing](#qemu-testing)
- [QEMU notes](#qemu-notes)
- [Included utilities](#included-utilities)
- [Contributing](#contributing)
- [Future plans](#future-plans)
- [Code statistics](#code-statistics)
- [Mirrors](#mirrors)
- [License](#license)

<!-- tocstop -->

---

## Overview

**CP/M‑386 is currently in the** ***very*** **early development stages.**

* Full 32‑bit [protected mode](https://en.wikipedia.org/wiki/Protected_mode)
  implementation with
  [Ring‑3 TPA](https://en.wikipedia.org/wiki/Protection_ring).
* Bootable via 3.5" 1.44MB floppy disk
  [boot sector](https://en.wikipedia.org/wiki/Boot_sector) or
  [Multiboot](https://en.wikipedia.org/wiki/Multiboot_specification) kernel.
* Supports [VGA text](https://en.wikipedia.org/wiki/VGA_text_mode) (`0xB8000`)
  and/or [COM1 serial](https://en.wikipedia.org/wiki/Serial_port)
  (9600/N/8/1, `0x3F8`) consoles.
* **No CD/USB/network/sound/other drivers** (**yet**).

## Hardware support

* Compatible with 386 (and later systems) with 2MB (or more) memory.
* Systems using either PC BIOS or UEFI (with CSM) are supported.
* VGA, 8042 PS/2, 8250/16450/16550 UART, CMOS RTC, and 8253/8254 PIT
  are supported.

## CP/M compatibility

**CP/M‑386** should be highly source‑compatible with other implementations:
|                System | BDOS coverage       |
|----------------------:|:--------------------|
| **CP/M‑68K&nbsp;1.3** | **100%**            |
| **CP/M&nbsp;2.2**     | **100%**            |
| **CP/M‑Plus**         | **74%**             |
| **DOS‑Plus**          | **62%**             |
| **MP/M&nbsp;2.1**     | **50%**             |

The system currently reports **BDOS 2.2** to applications.

* The **CP/M‑386** BDOS is at full parity with **CP/M‑68K&nbsp;1.3** and
  **CP/M&nbsp;2.2**.
* A large majority of the **CP/M‑Plus** (**CP/M&nbsp;3**) BDOS is also
  supported.
* More than 60% of the **DOS‑Plus** additions have been implemented.
* Approximately half of the **MP/M** extensions have been completed.
  * The missing functionality is largely the multi‑user, multi‑tasking,
    message queuing, and process control calls that don't apply to a
    single‑user **CP/M** implementation.
* Unique **CP/M‑386**‑specific BDOS extensions have been added to accommodate
  new features like direct video access, high‑resolution timing, PRNG, etc.

## Build requirements

The following dependencies are *required* to compile **CP/M‑386**:

* [AWK](https://en.wikipedia.org/wiki/AWK)
* [Cpmtools](https://www.moria.de/~michael/cpmtools/files)<sup>†</sup>
* [GNU Binutils](https://www.gnu.org/software/binutils/)
* [GNU Coreutils](https://www.gnu.org/software/coreutils/)
* [GNU GCC](https://gcc.gnu.org/) or [LLVM Clang](https://clang.llvm.org)
* [GNU Make](https://www.gnu.org/software/make/)
* [LZ4](https://lz4.org/)
* [NASM](https://nasm.us/)
[]()

[]()
<sup>†</sup>Be sure to use `cpmtools` version **2.23** or later.  Older
versions may *appear* to work but have several known bugs.

## Optional dependencies

* [diff](https://en.wikipedia.org/wiki/Diff)
* [QEMU](https://www.qemu.org)
* [sed](https://en.wikipedia.org/wiki/Sed)

## Downloads

* **Download the**
[**current CP/M‑386 build**](https://johnsonjh.gitlab.io/cpm386/cpm386.zip).
* View the
[GitLab CI/CD](https://gitlab.com/johnsonjh/cpm386/-/pipelines/latest) logs.

## Compilation

Building **CP/M‑386** is supported on the current releases of **NetBSD**
and **FreeBSD**<sup>‡</sup>, as well as Red Hat Enterprise Linux 9 (or later),
CentOS Stream 9 (or later), and Fedora 36 (or later).
[]()

[]()
* **GCC** build (recommended):
  ```sh
  make -Orecurse -j "$(nproc 2> /dev/null || printf '%s' 1)"
  ```
[]()

[]()
* **Clang** build:
  ```sh
  make -Orecurse -j "$(nproc 2> /dev/null || printf '%s' 1)" CC="clang" OPTFLAGS="-O1"
  ```
[]()

[]()
* It is recommended to use **GCC** as **Clang**‑compiled i386 code is larger.
  * **Clang**‑compiled builds will need to use `OPTFLAGS=-O1` (or `-Os` or
    `-Oz`) to avoid exceeding the 384 KiB allocated for the ramdisk, or the
    kernel size + Ring-0 stack exceeding conventional memory.
* Be sure to `make clean` if switching compilers or adjusting compiler flags.
* You may need to adjust the `make -j` argument depending on your operating
  system (*e.g.*, `gnproc`, `sysctl -n hw.ncpu`, `getconf NPROCESSORS_ONLN`,
  `psrinfo -p`).
* 32‑bit support libraries are required to run the test suite (`make test`).
* <sup>‡</sup>At the time of writing, **FreeBSD** is shipping non‑functional
  `cpmtools2` packages with broken `mkfs.cpm` functionality.  To successfully
  build on **FreeBSD**, you *must* rebuild `cpmtools` and ensure it is **not**
  linked with `libdsk`.  If you receive a `Disc rejected by driver` error on
  **FreeBSD** from `mkfs.cpm`, your tools are *broken* and *cannot* be used to
  build **CP/M‑386**.

## Docker build

If you are unable to build **CP/M‑386** natively on your system, a
Docker‑based Fedora build is available.
[]()

[]()
* First, build the `cpm386/cpm386-build` container:
  ```sh
  docker build --progress plain -t cpm386/cpm386-build:latest -f Dockerfile .
  ```
[]()

[]()
* Next, build **CP/M‑386** using this container:
  ```sh
  docker run --rm -v "$(pwd -P)":/src -w /src cpm386/cpm386-build:latest \
    make -Orecurse -j "$(nproc 2> /dev/null || printf '%s' 1)"
  ```
[]()

[]()
Additional targets (*i.e.*, `clean`, `test`, `lint`, `update-readme`) and
builds using the Clang compiler (`CC="clang" OPTFLAGS="-O1"`) are also
supported.

## Build output

* The build produces four primary artifacts:
  |          File | Description                              |
  |--------------:|:-----------------------------------------|
  | `cpm386.elf`  | Multiboot kernel image                   |
  | `floppy.img`  | Bootable 3.5" 1.44MB floppy disk image   |
  | `blankfd.img` | Blank CP/M 3.5" 1.44MB floppy disk image |
  | `blankhd.img` | Blank CP/M 8MB hard disk image           |
[]()

[]()
* The [`mboot.sh`](mboot.sh) convenience script (tested on GNU/Linux systems
  with GRUB2) can be used to create bootable multiboot media (such as a USB
  drive or SD card) using the `cpm386.elf` file.
* The `floppy.img` file can be written directly to a 1.44MB floppy disk.
  * **NOTE**: This disk image does *not* contain a CP/M filesystem!
    It can be removed from the drive once system is up and running.
* The blank images are useful because **CP/M‑386** does not yet have a
  `FORMAT` utility.

## QEMU testing

* [Multiboot](https://en.wikipedia.org/wiki/Multiboot_specification)
  kernel (recommended):
  ```sh
  qemu-system-i386 -m 2M -serial stdio -monitor none -kernel "cpm386.elf"
  ```
[]()

[]()
* Floppy [boot sector](https://en.wikipedia.org/wiki/Boot_sector) loader:
  ```sh
  qemu-system-i386 -m 2M -serial stdio -monitor none -drive if=floppy,format=raw,file="floppy.img" -boot a
  ```

## QEMU notes

* Use `-nographic -display none -vga none` to disable VGA video (and use *only* serial console).
* Use `-serial none` to disable the serial UART (and use *only* VGA console).
[]()

[]()
* To attach media to QEMU, use:
  ```sh
  -drive if=floppy,format=raw,file="blankfd.img"
  -drive if=ide,format=raw,file="blankhd.img",index=0
  ```

## Included utilities

|        Program | Description |
|---------------:|:------------|
| `ACLOCKDV.386` | [aclock](https://github.com/tenox7/aclock) (VGA text console version) |
| `ACLOCKVT.386` | [aclock](https://github.com/tenox7/aclock) (ANSI terminal version) |
| `ALVTST.386`   | Get Allocation Vector test (`DRV_ALLOCVEC`, [BDOS 27](https://www.seasip.info/Cpm/bdos.html)) |
| `CAPSLOCK.386` | Caps‑Lock key behavior utility (BDOS 235) |
| `CLEARTPA.386` | Clears (zeros) and optionally verifies the TPA |
| `CLS.386`      | Clear screen (BDOS 221) |
| `DELAY.386`    | Delay test (`P_DELAY`, [BDOS 141](https://www.seasip.info/Cpm/bdos.html#141)) |
| `DEMO.SUB`     | SUBMIT demonstration |
| `DUMPDIR.386`  | Directory entry dump utility (`F_SFIRST`/`F_SNEXT`, [BDOS 17](https://www.seasip.info/Cpm/bdos.html#17)/[18](https://www.seasip.info/Cpm/bdos.html#18)) |
| `DUMPFCB.386`  | File control block dump utility (`F_OPEN`, [BDOS 15](https://www.seasip.info/Cpm/bdos.html#15)) |
| `ED.386`       | ED (A WIP port of the DRI CP/M Context Editor, August 1982) |
| `ENV.DAT`      | Environment data file |
| `ESCTILDE.386` | Escape and Tilde key behavior utility (BDOS 237) |
| `FPARSE.386`   | Filename parsing test (`F_PARSE`, [BDOS 152](https://www.seasip.info/Cpm/bdos.html#152)) |
| `GETSN.386`    | Display serial number (`S_SERIAL`, [BDOS 107](https://www.seasip.info/Cpm/bdos.html#107)) |
| `GFXTEST.386`  | Graphics and framebuffer demo (BDOS 229/230/231/233) |
| `HD.386`       | Hex dump utility |
| `HELLO.386`    | Hello world - the first **CP/M‑386** program! (`C_WRITESTR`, [BDOS 9](https://www.seasip.info/Cpm/bdos.html#9)) |
| `ILLEGAL.386`  | Ring‑3 protection and exception handler test |
| `IOTEST.386`   | File I/O BDOS tests |
| `JULIA.386`    | Draw a Julia set fractal (terminal version) |
| `LRBC.386`     | Query and/or set Last Record Byte Count |
| `LS.386`       | List files (with sizes) |
| `MANDEL.386`   | Draw a Mandelbrot set fractal (terminal version) |
| `MEM.386`      | Memory map utility (BDOS 227/228) |
| `MORE.386`     | UNIX `more`‑style pager |
| `NUMLOCK.386`  | Num‑Lock key behavior utility (BDOS 236) |
| `OD.386`       | Octal dump utility |
| `PAUSE.386`    | Wait for keypress (`C_RAWIO`, [BDOS 6](https://www.seasip.info/Cpm/bdos.html#6)) |
| `PIP.386`      | PIP (A port of Zilog **CP/M-Z8000** PIP v1.0A, January 1984) |
| `PRINTENV.386` | Print environment and system data |
| `PRNG.386`     | PRNG test and demo utility (BDOS 253/254) |
| `PROFILE.SUB`  | SUBMIT script (automatically executed at boot) |
| `RC.386`       | Return code test and query (`P_CODE`, [BDOS 108](https://www.seasip.info/Cpm/bdos.html#108)) |
| `README.TXT`   | Sample text file |
| `REBOOT.386`   | Reboot utility (BDOS 220) |
| `RM.386`       | UNIX `rm`‑like interactive file deletion utility (`F_DELETE`, [BDOS 19](https://www.seasip.info/Cpm/bdos.html#19)) |
| `SEROFF.386`   | Disable serial console (BDOS 223) |
| `SERON.386`    | Enable serial console (BDOS 223) |
| `STAT.386`     | STAT (A port of Zilog **CP/M‑Z8000** STAT v1.0C January 1984) |
| `SYNC.386`     | Synchronize disks (`DRV_FLUSH`, [BDOS 48](https://www.seasip.info/Cpm/bdos.html#48)) |
| `TERMTEST.386` | Terminal and keyboard test utility |
| `TEST110.386`  | String delimiter test (`C_DELIMIT`, [BDOS 110](https://www.seasip.info/Cpm/bdos.html#110)) |
| `TEST211.386`  | Numeric format test (`C_DECNUM`, [BDOS 211](https://www.seasip.info/Cpm/bdos.html#211)) |
| `TEXTMODE.386` | Query and/or set the text mode and cursor state (BDOS 229/230/231/234) |
| `TICKS.386`    | High‑resolution timer tests (BDOS 225/226) |
| `TOD.386`      | Get (and set) Time of Day clock (`T_SET`/`T_GET`, [BDOS 104](https://www.seasip.info/Cpm/bdos.html#104)/[105](https://www.seasip.info/Cpm/bdos.html#105)) |
| `TOUCH.386`    | Create an empty file (`F_MAKE`, [BDOS 22](https://www.seasip.info/Cpm/bdos.html#22)) |
| `TRUNCATE.386` | File truncation utility (LRBC aware) |
| `TRUNCTST.386` | Truncation tests (`F_TRUNCATE`, [BDOS 99](https://www.seasip.info/Cpm/bdos.html#99)) |
| `TSEC.386`     | Get date and time (`T_SECONDS`, [BDOS 155](https://www.seasip.info/Cpm/bdos.html#155)) |
| `VER.386`      | Display OS version (`S_OSVER`, [BDOS 163](https://www.seasip.info/Cpm/bdos.html#163)) |
| `VGAFONT.386`  | Load a [text console font](https://github.com/viler-int10h/vga-text-mode-fonts) or restore the ROM font (BDOS 232) |
| `VGAOFF.386`   | Disable VGA text console (BDOS 222) |
| `VGAON.386`    | Enable VGA text console (BDOS 222) |
| `VGATEXT.386`  | VGA text direct access demo (BDOS 224) |

## Contributing

* Do **not** open pull requests with large amounts of LLM‑generated code.
  These will be immediately rejected.
* There is **no** AI‑generated code in the core operating system at
  this time (though there *are* AI‑generated tests, comments, and analysis),
  as the project is intended to be as much of a learning experience for me as
  it is a useful OS port.
* Usage of AI (artificial intelligence) tools by contributors *is* currently
  permitted, subject to the same terms and conditions as the
  [LLVM AI Tool Use Policy](https://llvm.org/docs/AIToolPolicy.html), but
  this permission may be withdrawn at any time and without notice.

## Future plans

See [FUTURE.md](FUTURE.md).

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
                <th>74</th>
                <th>46346</th>
                <th>8744</th>
                <th>7916</th>
                <th>29686</th>
                <th>6065</th>
                <th>1281899</th>
                <th>17655</th>
        </tr><tr>
                <th>C Header</th>
                <th>24</th>
                <th>3338</th>
                <th>646</th>
                <th>1481</th>
                <th>1211</th>
                <th>11</th>
                <th>123146</th>
                <th>1646</th>
        </tr><tr>
                <th>Assembly</th>
                <th>8</th>
                <th>1939</th>
                <th>339</th>
                <th>418</th>
                <th>1182</th>
                <th>1</th>
                <th>47942</th>
                <th>999</th>
        </tr><tr>
                <th>Makefile</th>
                <th>2</th>
                <th>1789</th>
                <th>342</th>
                <th>218</th>
                <th>1229</th>
                <th>376</th>
                <th>59508</th>
                <th>872</th>
        </tr><tr>
                <th>Markdown</th>
                <th>3</th>
                <th>626</th>
                <th>76</th>
                <th>0</th>
                <th>550</th>
                <th>0</th>
                <th>25989</th>
                <th>480</th>
        </tr><tr>
                <th>Shell</th>
                <th>1</th>
                <th>298</th>
                <th>88</th>
                <th>63</th>
                <th>147</th>
                <th>31</th>
                <th>8491</th>
                <th>116</th>
        </tr><tr>
                <th>Linker&amp;nbsp;Script</th>
                <th>2</th>
                <th>207</th>
                <th>39</th>
                <th>0</th>
                <th>168</th>
                <th>0</th>
                <th>5159</th>
                <th>96</th>
        </tr><tr>
                <th>Dockerfile</th>
                <th>1</th>
                <th>96</th>
                <th>8</th>
                <th>12</th>
                <th>76</th>
                <th>35</th>
                <th>3180</th>
                <th>81</th>
        </tr><tr>
                <th>YAML</th>
                <th>1</th>
                <th>87</th>
                <th>6</th>
                <th>15</th>
                <th>66</th>
                <th>0</th>
                <th>3850</th>
                <th>70</th>
        </tr><tr>
                <th>Plain Text</th>
                <th>1</th>
                <th>53</th>
                <th>0</th>
                <th>0</th>
                <th>53</th>
                <th>0</th>
                <th>923</th>
                <th>53</th>
        </tr></tbody>
        <tfoot><tr>
                <th>Total</th>
                <th>117</th>
                <th>54779</th>
                <th>10288</th>
                <th>10123</th>
                <th>34368</th>
                <th>6519</th>
                <th>1560087</th>
                <th>21961</th>
        </tr></tfoot></table>
<!-- scc-end -->

## Mirrors

* The canonical home of this software is
  [`https://gitlab.com/johnsonjh/cpm386`](https://gitlab.com/johnsonjh/cpm386),
  with a mirror on [GitHub](https://github.com/johnsonjh/cpm386).

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
