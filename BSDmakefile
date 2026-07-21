################################################################################
# CP/M-386 - BSDmakefile
# Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
# SPDX-License-Identifier: MIT
# scspell-id: 5f418f92-8475-11f1-88c0-80ee73e9b8e7
################################################################################

.PHONY: all
$(.TARGETS): all

################################################################################

all:
	@tput bold 2> /dev/null || :
	@tput setaf 1 2> /dev/null || :
	@command -v gmake > /dev/null 2>&1 && { printf '\r\n%s\r\n\r\n' \
	 "ERROR: BSD make is unsupported; use gmake ($$(command -v gmake))" \
	  >&2; }
	@command -v gmake > /dev/null 2>&1 || { printf '\r\n%s\r\n\r\n' \
	 "ERROR: BSD make is unsupported; you must install GNU make (gmake)." \
	  >&2; }
	@tput sgr0 2> /dev/null || :
	@false > /dev/null 2>&1

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
