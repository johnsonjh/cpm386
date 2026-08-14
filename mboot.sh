#!/bin/sh
###############################################################################
# CP/M-386 - mboot.sh
# Copyright (c) 2026 Jeffrey H. Johnson
# SPDX-License-Identifier: MIT
# scspell-id: 45ecfa78-93e2-11f1-92f7-80ee73e9b8e7
###############################################################################

command -v command > /dev/null 2>&1 || {
  echo 'ERROR: "command" not available!'
  exit 1
}

###############################################################################

command -v printf > /dev/null 2>&1 || {
  echo "ERROR: printf command not available!"
  exit 1
}

###############################################################################

test -z "$1" && {
  printf '%s\n' 'ERROR: Must specify target device (e.g., "/dev/sde")!'
  exit 1
}

###############################################################################

test -b "$1" || {
  printf '%s\n' "ERROR: \"$1\" is not a block device!"
  exit 1
}

###############################################################################

command -v wipefs > /dev/null 2>&1 || {
  printf '%s\n' "ERROR: wipefs command not available!"
  exit 1
}

###############################################################################

command -v parted > /dev/null 2>&1 || {
  printf '%s\n' "ERROR: parted command not available!"
  exit 1
}

###############################################################################

command -v mkfs.fat > /dev/null 2>&1 || {
  printf '%s\n' "ERROR: mkfs.fat command not available! (install dosfstools)"
  exit 1
}

###############################################################################

GRUB_INSTALL=$(command -v grub2-install 2> /dev/null \
  || command -v grub-install 2> /dev/null || printf '%s\n' "grub2-install")

###############################################################################

command -v "${GRUB_INSTALL:?}" > /dev/null 2>&1 || {
  printf '%s\n' "ERROR: grub2-install command not available!"
  exit 1
}

###############################################################################

command -v mount > /dev/null 2>&1 || {
  printf '%s\n' "ERROR: mount command not available!"
  exit 1
}

###############################################################################

command -v umount > /dev/null 2>&1 || {
  printf '%s\n' "ERROR: umount command not available!"
  exit 1
}

###############################################################################

command -v mkdir > /dev/null 2>&1 || {
  printf '%s\n' "ERROR: mkdir command not available!"
  exit 1
}

###############################################################################

command -v tee > /dev/null 2>&1 || {
  printf '%s\n' "ERROR: tee command not available!"
  exit 1
}

###############################################################################

command -v cp > /dev/null 2>&1 || {
  printf '%s\n' "ERROR: cp command not available!"
  exit 1
}

###############################################################################

command -v sleep > /dev/null 2>&1 || {
  printf '%s\n' "ERROR: sleep command not available!"
  exit 1
}

###############################################################################

command -v id > /dev/null 2>&1 || {
  printf '%s\n' "ERROR: id command not available!"
  exit 1
}

###############################################################################

command -v sync > /dev/null 2>&1 || {
  printf '%s\n' "ERROR: sync command not available!"
  exit 1
}

###############################################################################

SED=$(command -v gsed 2> /dev/null || command -v sed 2> /dev/null \
  || printf '%s\n' "sed")

###############################################################################

command -v "${SED:?}" > /dev/null 2>&1 || {
  printf '%s\n' "ERROR: ${SED:?} command not available!"
  exit 1
}

###############################################################################

command -v rmdir > /dev/null 2>&1 || {
  printf '%s\n' "ERROR: rmdir command not available!"
  exit 1
}

###############################################################################

command -v uname > /dev/null 2>&1 || {
  printf '%s\n' "ERROR: uname command not available!"
  exit 1
}

###############################################################################

test "$(id -u || :)" = "0" || {
  printf '%s\n' "ERROR: script must run as root!"
  exit 1
}

###############################################################################

test "$(uname -s || :)" = "Linux" || {
  printf '%s\n' "WARNING: This script has only been tested on GNU/Linux."
  sleep 2
}

###############################################################################

umount /mnt/cpm386 > /dev/null 2>&1
rmdir /mnt/cpm386 > /dev/null 2>&1

###############################################################################

test -d "/mnt/cpm386" && {
  printf '%s\n' 'ERROR: "/mnt/cpm386" already exists!'
  exit 1
}

###############################################################################

test -f ./cpm386.elf || {
  printf '%s\n' 'ERROR: "./cpm386.elf" does not exist!'
  exit 1
}

###############################################################################

device="$(printf '%s' "$1" | "${SED:?}" 's/^[ ]*//; s/[ ]*$//')"

###############################################################################

printf '%s\n' ""
printf '%s\n' "CP/M-386 Multiboot Image Writer"
printf '%s\n' "==============================="
printf '%s\n' ""
printf '%s\n' "WARNING: All existing data on \"${device:?}\" will be DESTROYED."
printf '%s\n' "         Interrupt (^C) within 5 seconds if this is incorrect!"
sleep 5 > /dev/null 2>&1
printf '%s\n' ""

###############################################################################

set -eux

###############################################################################

wipefs -a "${device:?}"

###############################################################################

parted -f -s "${device:?}" mklabel gpt

###############################################################################

parted -f -s "${device:?}" mkpart "BIOS_Boot" 17KiB 150KiB
parted -f -s "${device:?}" set 1 bios_grub on

###############################################################################

parted -f -s "${device:?}" mkpart "ESP" fat16 150KiB 1423KiB
parted -f -s "${device:?}" set 2 esp on

###############################################################################

mkfs.fat -v -F 12 -n "CPM386" "${device:?}2"

###############################################################################

mkdir -p "/mnt/cpm386"
mount "${device:?}2" "/mnt/cpm386"

###############################################################################

mkdir -p "/mnt/cpm386/empty"
mkdir -p "/mnt/cpm386/boot"
cp "./cpm386.elf" "/mnt/cpm386/boot/"

###############################################################################

"${GRUB_INSTALL:?}" \
  --boot-directory="/mnt/cpm386/boot" \
  --fonts="" \
  --install-modules="part_gpt fat multiboot normal boot minicmd" \
  --locale-dir="/mnt/cpm386/empty" \
  --recheck "${device:?}" \
  --target=i386-pc

###############################################################################

tee "/mnt/cpm386/boot/grub2/grub.cfg" << 'EOF'
set timeout=5
set default=0

insmod fat
insmod multiboot
insmod part_gpt

menuentry "CP/M-386" {
  multiboot /boot/cpm386.elf
  boot
}
EOF

###############################################################################

sync

###############################################################################

umount /mnt/cpm386

###############################################################################

set +x

###############################################################################

printf '%s\n' ""
printf '%s\n' "Script finished."

###############################################################################
# Local Variables:
# mode: shell
# indent-tabs-mode: nil
# sh-basic-offset: 2
# tab-width: 2
# fill-column: 80
# eval: (add-hook 'before-save-hook 'untabify nil t)
# eval: (setq-local display-fill-column-indicator-column 80)
# eval: (display-fill-column-indicator-mode 1)
# End:
###############################################################################
# vim: set ft=sh expandtab tabstop=2 cc=80 :
###############################################################################
