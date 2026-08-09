#!/bin/sh

test -z "$1" && {
  printf '%s\n' "ERROR: Must specify target device, i.e., \"/dev/sde\"!"
  exit 1
}

test -b "$1" || {
  printf '%s\n' "ERROR: \"$1\" is not a block device!"
  exit 1
}

command -v wipefs > /dev/null 2>&1 || {
  printf '%s\n' "ERROR: wipefs command not available!"
  exit 1
}

command -v parted > /dev/null 2>&1 || {
  printf '%s\n' "ERROR: parted command not available!"
  exit 1
}

command -v mkfs.fat > /dev/null 2>&1 || {
  printf '%s\n' "ERROR: mkfs.fat command not available! (install dosfstools)"
  exit 1
}

command -v grub2-install > /dev/null 2>&1 || {
  printf '%s\n' "ERROR: grub2-install command not available!"
  exit 1
}

command -v mount > /dev/null 2>&1 || {
  printf '%s\n' "ERROR: mount command not available!"
  exit 1
}

command -v umount > /dev/null 2>&1 || {
  printf '%s\n' "ERROR: umount command not available!"
  exit 1
}

command -v mkdir > /dev/null 2>&1 || {
  printf '%s\n' "ERROR: mkdir command not available!"
  exit 1
}

command -v tee > /dev/null 2>&1 || {
  printf '%s\n' "ERROR: tee command not available!"
  exit 1
}

command -v cp > /dev/null 2>&1 || {
  printf '%s\n' "ERROR: cp command not available!"
  exit 1
}

command -v sleep > /dev/null 2>&1 || {
  printf '%s\n' "ERROR: sleep command not available!"
  exit 1
}

command -v id > /dev/null 2>&1 || {
  printf '%s\n' "ERROR: id command not available!"
  exit 1
}

command -v sync > /dev/null 2>&1 || {
  printf '%s\n' "ERROR: sync command not available!"
  exit 1
}

command -v rmdir > /dev/null 2>&1 || {
  printf '%s\n' "ERROR: rmdir command not available!"
  exit 1
}

test "$(id -u || :)" = "0" || {
  printf '%s\n' "ERROR: script must run as root!"
  exit 1
}

umount /mnt/sdcard > /dev/null 2>&1
rmdir /mnt/sdcard > /dev/null 2>&1

test -d "/mnt/sdcard" && {
  printf '%s\n' "ERROR: \"/mnt/sdcard\" already exists!"
  exit 1
}

test -f ./cpm386.elf || {
  printf '%s\n' "ERROR: \"./cpm386.elf\" does not exist!"
  exit 1
}

device="$(printf '%s' "$1" | sed 's/^[ ]*//; s/[ ]*$//')"

printf '%s\n' "CP/M-386 Multiboot Image Writer"
printf '%s\n' "==============================="
printf '%s\n' ""
printf '%s\n' "WARNING: Any data existing on \"${device:?}\" will be DESTROYED."
printf '%s\n' "         Interrupt (^C) within 5s if this is incorrect."
sleep 5 > /dev/null 2>&1
printf '%s\n' ""

set -eux

set -eux

wipefs -a "${device:?}"

parted -s "${device:?}" mklabel gpt

parted -s "${device:?}" mkpart "BIOS_Boot" 1MiB 2MiB
parted -s "${device:?}" set 1 bios_grub on

parted -s "${device:?}" mkpart "ESP" fat32 2MiB 100%
parted -s "${device:?}" set 2 esp on

mkfs.fat -v -F 32 -n "CPM386" "${device:?}2"

mkdir -p "/mnt/sdcard"
mount "${device:?}2" "/mnt/sdcard"

mkdir -p "/mnt/sdcard/boot"
cp "./cpm386.elf" "/mnt/sdcard/boot/"

grub2-install \
  --boot-directory="/mnt/sdcard/boot" \
  --recheck "${device:?}" \
  --target=i386-pc

tee "/mnt/sdcard/boot/grub2/grub.cfg" << 'EOF'
set timeout=5
set default=0

insmod multiboot

menuentry "CP/M-386" {
    multiboot /boot/cpm386.elf
    boot
}
EOF

sync

umount /mnt/sdcard

set +x

printf '%s\n' ""
printf '%s\n' "Script finished."
