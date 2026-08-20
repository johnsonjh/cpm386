################################################################################
# CP/M-386 - Dockerfile
# Copyright (c) 2026 Jeffrey H. Johnson <johnsonjh.dev@gmail.com>
# SPDX-License-Identifier: MIT
# scspell-id: 962253a8-9446-11f1-bed3-80ee73e9b8e7
################################################################################

FROM quay.io/fedora/fedora:rawhide

################################################################################

ENV HOME="/root"
ENV PATH="${HOME:?}/.local/bin:${HOME:?}/go/bin:${PATH:-}"
ENV TERM="xterm-color"

################################################################################

RUN \
  set -eux && \
  mkdir -p "${HOME:?}" && \
  dnf -y upgrade \
    --allowerasing \
    --setopt=keepcache=True && \
  dnf -y install \
    bash \
    binutils \
    clang \
    codespell \
    coreutils \
    cpmtools \
    cppi \
    curl \
    devscripts-checkbashisms \
    file \
    file-devel \
    gawk \
    gcc \
    git \
    glibc-devel.i686 \
    glibc.i686 \
    golang \
    libatomic.i686 \
    libatomic.x86_64 \
    lz4 \
    make \
    nasm \
    pigz \
    python3 \
    python3-pip \
      --allowerasing && \
  curl -fsSL "https://rpm.nodesource.com/setup_26.x" | bash - && \
  dnf install -y nodejs && \
  { dnf -y clean all || :; } && \
  mkdir -p "/etc/apt/apt.conf.d" && \
  export PATH="${HOME:?}/go/bin:${PATH:-}" && \
  export GOPROXY="proxy.golang.org,direct" && \
  export GOSUMDB="sum.golang.org" && \
  export GOTOOLCHAIN="auto" && \
  go install "github.com/boyter/scc/v3@master" && \
  go install "mvdan.cc/sh/v3/cmd/shfmt@master" && \
  { go clean -modcache -testcache -cache 2> /dev/null || :; } && \
  mkdir -vp "${HOME:?}/.local/venvs" && \
  mkdir -vp "${HOME:?}/.local/bin" && \
  rm -vrf "${HOME:?}/.local/venvs/codespell" && \
  python3 -m venv "${HOME:?}/.local/venvs/codespell" && \
  "${HOME:?}/.local/venvs/codespell/bin/pip" install --upgrade "pip" && \
  "${HOME:?}/.local/venvs/codespell/bin/pip" install --upgrade "codespell" && \
  printf '%s\n' \
    '#!/bin/sh' \
    'exec "${HOME:?}/.local/venvs/codespell/bin/codespell" "$@"' \
      > "${HOME:?}/.local/bin/codespell" && \
  chmod -v a+x "${HOME:?}/.local/bin/codespell" && \
  rm -vrf "${HOME:?}/.local/venvs/reuse" && \
  python3 -m venv "${HOME:?}/.local/venvs/reuse" && \
  "${HOME:?}/.local/venvs/reuse/bin/pip" install --upgrade "pip" && \
  "${HOME:?}/.local/venvs/reuse/bin/pip" install --upgrade "reuse" && \
  printf '%s\n' \
    '#!/bin/sh' \
    'exec "${HOME:?}/.local/venvs/reuse/bin/reuse" "$@"' \
      > "${HOME:?}/.local/bin/reuse" && \
  chmod -v a+x "${HOME:?}/.local/bin/reuse" && \
  npm config set fund "false" && \
  npm config set audit "false" && \
  npm install -g "npm@latest" && \
  npm install -g "markdown-toc" && \
  rm -vrf /var/log/dnf*.log "${HOME:?}/.npm/"* "${HOME:?}/.cache/"*

################################################################################

WORKDIR /src

################################################################################
# vim: set ft=dockerfile cc=80 :
################################################################################
