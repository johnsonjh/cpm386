# Future plans

<!-- Copyright (c) 2026 Jeffrey H. Johnson -->
<!-- SPDX-License-Identifier: MIT -->
<!-- scspell-id: 2e3ddd38-8367-11f1-a221-80ee73e9b8e7 -->

I hope to eventually implement the following features in **CP/M‑386**.  Treat
this section as *wishlist* and not a *roadmap*!

* OS enhancements:
  * Finish remaining unimplemented BDOS functions
  * Drivers for floppy disk, hard disk, and parallel printer
  * PRNG (based on interrupt timer/PIT first, detect RDRAND/RDSEED later)
  * Better keyboard support (Alt+nnn‑style 8‑bit input, numeric keypad,
    function keys, arrow/motion keys, etc.)
  * User‑defined configurable first‑class RAM disk (default to M:)
  * Disk caching
  * Drivers for networking (with CP/Net NDOS and generic packet interface)
  * DOS‑PLUS (enhanced VT52) compatible VGA console (with optional ADM3A mode)
  * Enhance filesystem with CP/M‑Plus parity (timestamps, passwords, etc.)
  * Extend user area support from 16 to 32 user areas per disk
  * Extend direct video support for mode switching and framebuffer
    (Doom port!)
  * Support for non‑PIT high‑resolution timers (HPET, APIC, TSC, RDTSC)
  * ZCPR TCAP‑like standard terminal capability database
  * DOS‑PLUS / CP/M‑Plus style status line with user customization
  * GSX graphics (mostly standard GSX‑stack of GDD, GIOS, GDOS, etc.)

* Transparent emulation:
  * Transparent CP/M‑80 Z80 emulation for CP/M‑80 `COM` programs
  * Transparent CP/M‑86 execution environment for CP/M‑86 `CMD` programs
    (utilizing V86 mode to avoid emulation overhead)
  * Transparent Heath [HDOS](https://en.wikipedia.org/wiki/HDOS) 8080
    emulation for HDOS `ABS` programs
    ([HRUN](https://heathkit.garlanger.com/software/library/HUG/docs/HRUN_doc.pdf)
    is prior art, also see this
    [article](https://dl.acm.org/doi/pdf/10.1145/358061.358070?download=true))

* CCP enhancements:
  * Allow "/" in file names (*e.g*, "CP/M.TXT", "SYS/STAT.H")
  * Move the CCP out of Ring 0 to Ring 2
  * Command/file completion
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
  * Extend MANDEL and JULIA to support arbitrary resolution via command tail
  * VGA framebuffer version of MANDEL and JULIA supporting many resolutions
  * Add return status to all software ports
  * Extend VER to allow changing the reported BDOS version
  * Expand LS to show timestamps
  * Add SAVE program to write TPA to disk, and to optionally add a `.386`
    executable header.  SAVE should be able to take a range to save, as well
    as the ability to autodetect a range (`CLEARTPA` could be useful here).
  * Extend STAT to query and also set the LRBC
  * Digital Research ED (port Zilog CP/M‑Z8000 ED 08/1982)
  * Digital Research PIP (port Zilog CP/M‑Z8000 PIP 1.0A 01/03/1984) +LRBC
  * Digital Research HELP clone (VMS‑style HELP tool)
  * [wumpus](https://gitlab.com/esr/wumpus/-/raw/master/wumpus.c) or
    [this one](https://codeberg.org/EtchedPixels/FUZIX/src/commit/c80213af16cd4c948e0afb3477fe5584762aa1a7/Applications/V7/games/wump.c)
    port
  * less‑style pager
  * [6FORTH](https://github.com/johnsonjh/6FORTH) port
  * [`pc`](https://github.com/johnsonjh/pc) port
  * [startrek](https://github.com/davidgiven/cpmish/blob/master/third_party/startrek/startrek.c)
    port
  * UNIX `bc` port
  * UNIX‑style `cp` copy command that supports user areas, LRBC, etc.
  * CPUID‑type utility
  * Compression tools (`arc`, `lha`, `gzip`, `compress`, `zip`, etc.)
    w/LRBC support
  * [TPZASM](https://github.com/johnsonjh/tpzasm) port
  * [T3X/0](https://www.t3x.org/t3x/0/index_d.html)
  * Kermit and YAM ports (requires a proper IOBYTE implementation!)
  * Nice hex editor (supporting direct modification of the TPA)
  * Editors! MINCE, uEmacs, CALVIN, `g`, `s`, `te`,
    [VEDIT‑PLUS](https://github.com/johnsonjh/VEDIT)?
  * [Ack](https://github.com/davidgiven/ack) CP/M‑386 target
  * More tools from
    [tsupplis](https://github.com/tsupplis/cpm86-hacking)
  * A native DDT‑like debugger
  * A real assembler (most likely [NASM](https://nasm.us/))
  * A real C standard library customized for CP/M‑386
    (probably [newlib](https://sourceware.org/git/newlib-cygwin.git))
  * A real CP/M‑386 native C compiler

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
