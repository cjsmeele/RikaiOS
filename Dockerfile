FROM binhex/arch-base:latest
MAINTAINER peikos

RUN pacman --noconfirm -S llvm lld clang compiler-rt nasm make parted qemu qemu-arch-extra mtools
RUN mkdir -p src
WORKDIR src
ENTRYPOINT ["make"]
CMD ["disk"]
