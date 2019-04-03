FROM binhex/arch-base:latest
MAINTAINER peikos

RUN pacman --noconfirm -S llvm lld clang nasm make parted qemu qemu-arch-extra
RUN mkdir -p src
WORKDIR src
ENTRYPOINT ["make"]
CMD ["disk"]
