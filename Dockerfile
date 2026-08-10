################################################################################
# CP/M-386 - Dockerfile
# Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
# SPDX-License-Identifier: MIT
# scspell-id: 962253a8-9446-11f1-bed3-80ee73e9b8e7
################################################################################

FROM quay.io/fedora/fedora:rawhide

################################################################################

RUN \
  dnf -y upgrade \
    --allowerasing \
    --setopt=install_weak_deps=True \
    --setopt=keepcache=True && \
  dnf -y install \
    binutils \
    coreutils \
    cpmtools \
    gawk \
    gcc \
    git \
    glibc-devel.i686 \
    glibc.i686 \
    libatomic.i686 \
    libatomic.x86_64 \
    lz4 \
    make \
    nasm \
    pigz \
      --allowerasing \
      --setopt=install_weak_deps=True && \
  { dnf -y clean all || true; } && \
  rm -f /var/log/dnf*.log

################################################################################

WORKDIR /src

################################################################################

CMD ["make"]

################################################################################
# vim: set ft=dockerfile cc=80 :
################################################################################
