#!/usr/bin/env python3
"""Checks a Linux plugin binary built for SA-MP/open.mp.

Verifies it is a 32-bit x86 shared object that exports the plugin and
component entry points, and prints its dynamic dependencies and the newest
glibc symbol version it requires. Standard library only, runs anywhere.

    python tools/elfcheck.py dist/plugins/weapon-forge.so
"""
import struct
import sys

REQUIRED_EXPORTS = ["Supports", "Load", "Unload", "AmxLoad", "AmxUnload", "ProcessTick", "ComponentEntryPoint"]


def main(path):
    data = open(path, "rb").read()

    if data[:4] != b"\x7fELF":
        return fail("not an ELF file")
    if data[4] != 1:
        return fail("not a 32-bit ELF (SA-MP/open.mp plugins must be 32-bit)")

    e_type, e_machine = struct.unpack_from("<HH", data, 16)
    if e_type != 3:
        return fail("not a shared object")
    if e_machine != 3:
        return fail("not an x86 (i386) binary")

    e_shoff, = struct.unpack_from("<I", data, 32)
    e_shentsize, e_shnum = struct.unpack_from("<HH", data, 46)

    sections = []
    for i in range(e_shnum):
        sh = struct.unpack_from("<IIIIIIIIII", data, e_shoff + i * e_shentsize)
        sections.append(sh)

    def section_bytes(sh):
        return data[sh[4]:sh[4] + sh[5]]

    def cstr(blob, offset):
        end = blob.index(b"\0", offset)
        return blob[offset:end].decode()

    exports = set()
    needed = []
    glibc = []

    for sh in sections:
        sh_type = sh[1]

        if sh_type == 11:  # SHT_DYNSYM
            strtab = section_bytes(sections[sh[6]])
            symbols = section_bytes(sh)
            for off in range(0, len(symbols), 16):
                st_name, st_value, st_size, st_info, st_other, st_shndx = struct.unpack_from("<IIIBBH", symbols, off)
                if st_shndx != 0 and st_name:
                    exports.add(cstr(strtab, st_name))

        elif sh_type == 6:  # SHT_DYNAMIC
            strtab = section_bytes(sections[sh[6]])
            dyn = section_bytes(sh)
            for off in range(0, len(dyn), 8):
                tag, val = struct.unpack_from("<iI", dyn, off)
                if tag == 1:  # DT_NEEDED
                    needed.append(cstr(strtab, val))

        elif sh_type == 0x6FFFFFFE:  # SHT_GNU_verneed
            strtab = section_bytes(sections[sh[6]])
            blob = section_bytes(sh)
            off = 0
            while True:
                vn_version, vn_cnt, vn_file, vn_aux, vn_next = struct.unpack_from("<HHIII", blob, off)
                aux = off + vn_aux
                for _ in range(vn_cnt):
                    vna_hash, vna_flags, vna_other, vna_name, vna_next = struct.unpack_from("<IHHII", blob, aux)
                    name = cstr(strtab, vna_name)
                    if name.startswith("GLIBC_"):
                        glibc.append(name)
                    if not vna_next:
                        break
                    aux += vna_next
                if not vn_next:
                    break
                off += vn_next

    missing = [name for name in REQUIRED_EXPORTS if name not in exports]
    if missing:
        return fail("missing exports: " + ", ".join(missing))

    def version_key(v):
        return tuple(int(x) for x in v[len("GLIBC_"):].split(".") if x.isdigit())

    print("ELF 32-bit x86 shared object: OK")
    print("Exports: " + ", ".join(REQUIRED_EXPORTS))
    print("Needs: " + (", ".join(needed) or "-"))
    print("Newest glibc symbol: " + (max(glibc, key=version_key) if glibc else "-"))
    return 0


def fail(message):
    print("ERROR: " + message)
    return 1


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print(__doc__)
        sys.exit(2)
    sys.exit(main(sys.argv[1]))
