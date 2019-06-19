#!/bin/sh

# This creates a partitioned disk image for use in an emulator.
# Installing the OS on this disk is a separate step - see
# boot/utils/loader-install or use the Makefile in the current directory.

# Abort immediately when errors occur.
set -e

# By default, reserve 8M of disk space for the bootloader and kernel code.
: ${BOOTPART_SIZE:=8}

if [ $# != 2 ]; then
    echo "usage: $0 <disk_file> <size_in_MB>" >&2
    echo "(size should be at least 16)" >&2
else
    truncate -s "${2}M" "$1"

    # Create a 8M boot partition for the bootloader and the kernel.
    # The rest of the disk will be usable by the OS.
    parted -s                                             \
        "$1"                                              \
        unit MiB                                          \
        mklabel msdos                                     \
        mkpart  primary ext2 1 $(($BOOTPART_SIZE+1))      \
        mkpart  primary fat32  $(($BOOTPART_SIZE+1)) '100%'

    # Hackily overwrite the partition id (fs type) of the first partition.
    # This is to prevent anyone from trying to mount it (it's not a filesystem).
    # (parted does not allow us to set such an fs type).
    printf '\x7f' | dd status=none bs=1 of="$1" seek=$((0x1c2)) conv=notrunc

    # Format FAT partition (tell mformat where the partition starts and its size).
    mformat -i "$1"@@$(($BOOTPART_SIZE+1))M -T $((($2-$BOOTPART_SIZE-1)*1024*2)) -v EOSOS -F
    mcopy   -i "$1"@@$(($BOOTPART_SIZE+1))M -s disk/*     ::/
    mcopy   -i "$1"@@$(($BOOTPART_SIZE+1))M -s user/bin/* ::/
fi
