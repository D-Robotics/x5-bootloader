FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        sudo \
        git vim parted \
        tzdata tree bc hashdeep kmod file wget curl cpio unzip rsync liblz4-tool \
        build-essential make cmake bison flex ccache zlib1g-dev libssl-dev \
        libncurses-dev u-boot-tools device-tree-compiler \
        android-sdk-libsparse-utils dosfstools mtools mtd-utils e2fsprogs \
        python3 python3-pip \
        rsync grep udev xz-utils curl xxd file kmod bc \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install --no-cache-dir --no-build-isolation cryptography==40.0.2

RUN apt-get autoremove -y \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*
