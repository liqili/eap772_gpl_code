#!/usr/bin/env python3
"""Write a firmware rootfs as a tar stream for `docker import`, without unsquashfs.

usage: sqfs2tar.py SOURCE [OUT.tar]      (OUT defaults to stdout)
  SOURCE is a squashfs 4.0 image (xz or gzip, padding after it is fine), for example the
  built ipq53xx_eap7xx_common-squashfs-root.img or the rootfs volume of a flash dump, or a
  rootfs directory such as the tree's images/eap/ipq53xx_eap7xx_common/rootfs.
Entries keep their mode, owner (squashfs id table; root for a directory) and mtime. Device
nodes, fifos and symlinks are kept; sockets are skipped (tar cannot hold them). Hard links
become separate copies. Pure Python 3, standard library only.
"""
import io
import lzma
import os
import stat
import struct
import sys
import tarfile
import zlib


class SquashFS:
    def __init__(self, path):
        with open(path, 'rb') as f:
            self.b = f.read()
        (magic, self.inodes, _mt, self.bsize, _frags, self.comp, _blog, _flags, id_count,
         vmaj, vmin, self.root_ref, _used, id_t, _xattr_t, self.ino_t, self.dir_t,
         self.frag_t, _exp_t) = struct.unpack_from('<IIIIIHHHHHHQQQQQQQQ', self.b, 0)
        if magic != 0x73717368 or (vmaj, vmin) != (4, 0):
            raise SystemExit('%s: not a squashfs 4.0 image' % path)
        if self.comp not in (1, 4):
            raise SystemExit('%s: compression id %d is not supported (gzip=1, xz=4)' % (path, self.comp))
        self.blocks = {}
        raw = b''.join(self.meta_block(struct.unpack_from('<Q', self.b, id_t + 8 * i)[0])[0]
                       for i in range((id_count * 4 + 8191) // 8192))
        self.ids = struct.unpack_from('<%dI' % id_count, raw)

    def dec(self, data):
        return lzma.decompress(data, format=lzma.FORMAT_XZ) if self.comp == 4 else zlib.decompress(data)

    def meta_block(self, pos):
        """(uncompressed bytes, position of the next block) for the metadata block at pos."""
        if pos not in self.blocks:
            hdr, = struct.unpack_from('<H', self.b, pos)
            size = hdr & 0x7FFF
            raw = self.b[pos + 2: pos + 2 + size]
            self.blocks[pos] = (raw if hdr & 0x8000 else self.dec(raw), pos + 2 + size)
        return self.blocks[pos]

    def meta(self, table, blk, off, n):
        out, pos = bytearray(), table + blk
        while len(out) < off + n:
            data, pos = self.meta_block(pos)
            out += data
        return bytes(out[off: off + n])

    def inode(self, ref):
        blk, off = ref >> 16, ref & 0xFFFF
        head = self.meta(self.ino_t, blk, off, 64)
        t, = struct.unpack_from('<H', head, 0)
        if t in (2, 9):  # files: re-read with room for the block list
            if t == 2:
                frag, fsize = struct.unpack_from('<II', head, 20)[0], struct.unpack_from('<I', head, 28)[0]
                hdr = 32
            else:
                fsize, = struct.unpack_from('<Q', head, 24)
                frag, = struct.unpack_from('<I', head, 44)
                hdr = 56
            nblk = fsize // self.bsize if frag != 0xFFFFFFFF else (fsize + self.bsize - 1) // self.bsize
            return self.meta(self.ino_t, blk, off, hdr + 4 * nblk)
        if t in (3, 10):  # symlinks: re-read with room for the target
            size, = struct.unpack_from('<I', head, 20)
            return self.meta(self.ino_t, blk, off, 24 + size)
        return head

    def listdir(self, ino):
        t, = struct.unpack_from('<H', ino, 0)
        if t == 1:
            sblk, _nl, fsize, off, _par = struct.unpack_from('<IIHHI', ino, 16)
        else:
            _nl, fsize, sblk, _par, _ic, off = struct.unpack_from('<IIIIHH', ino, 16)
        if fsize <= 3:
            return []
        data, pos, ents = self.meta(self.dir_t, sblk, off, fsize - 3), 0, []
        while pos < len(data):
            cnt, start, _base = struct.unpack_from('<III', data, pos)
            pos += 12
            for _ in range(cnt + 1):
                eoff, _delta, _t, nsz = struct.unpack_from('<HhHH', data, pos)
                pos += 8
                ents.append((data[pos: pos + nsz + 1].decode('utf-8', 'surrogateescape'), (start << 16) | eoff))
                pos += nsz + 1
        return ents

    def file_data(self, ino):
        t, = struct.unpack_from('<H', ino, 0)
        if t == 2:
            start, frag, foff, fsize = struct.unpack_from('<IIII', ino, 16)
            hdr = 32
        else:
            start, fsize, _sp, _nl, frag, foff, _x = struct.unpack_from('<QQQIIII', ino, 16)
            hdr = 56
        nblk = fsize // self.bsize if frag != 0xFFFFFFFF else (fsize + self.bsize - 1) // self.bsize
        out, pos = bytearray(), start
        for i in range(nblk):
            sz, = struct.unpack_from('<I', ino, hdr + 4 * i)
            n = sz & 0xFFFFFF
            if n == 0:  # sparse block
                out += bytes(min(self.bsize, fsize - len(out)))
                continue
            chunk = self.b[pos: pos + n]
            out += chunk if sz & 0x1000000 else self.dec(chunk)
            pos += n
        if frag != 0xFFFFFFFF:
            ptr, = struct.unpack_from('<Q', self.b, self.frag_t + (frag // 512) * 8)
            fstart, fsz, _u = struct.unpack_from('<QII', self.meta(ptr, 0, (frag % 512) * 16, 16), 0)
            n = fsz & 0xFFFFFF
            chunk = self.b[fstart: fstart + n]
            fb = chunk if fsz & 0x1000000 else self.dec(chunk)
            out += fb[foff: foff + (fsize - len(out))]
        return bytes(out[:fsize])

    def walk(self, ref=None, prefix='.'):
        """Yield (path, inode bytes) depth first, directories before their contents."""
        ref = self.root_ref if ref is None else ref
        ino = self.inode(ref)
        yield prefix, ino
        t, = struct.unpack_from('<H', ino, 0)
        if t in (1, 8):
            for name, child in self.listdir(ino):
                yield from self.walk(child, prefix + '/' + name)


def tar_squashfs(src, tar):
    fs = SquashFS(src)
    kinds = {1: tarfile.DIRTYPE, 8: tarfile.DIRTYPE, 2: tarfile.REGTYPE, 9: tarfile.REGTYPE,
             3: tarfile.SYMTYPE, 10: tarfile.SYMTYPE, 4: tarfile.BLKTYPE, 11: tarfile.BLKTYPE,
             5: tarfile.CHRTYPE, 12: tarfile.CHRTYPE, 6: tarfile.FIFOTYPE, 13: tarfile.FIFOTYPE}
    count = 0
    for path, ino in fs.walk():
        t, mode, uid_i, gid_i, mtime, _num = struct.unpack_from('<HHHHII', ino, 0)
        if t not in kinds:  # sockets
            continue
        ti = tarfile.TarInfo(path)
        ti.type, ti.mode, ti.mtime = kinds[t], mode & 0o7777, mtime
        ti.uid, ti.gid = fs.ids[uid_i], fs.ids[gid_i]
        ti.uname = ti.gname = ''
        data = None
        if t in (2, 9):
            data = fs.file_data(ino)
            ti.size = len(data)
        elif t in (3, 10):
            size, = struct.unpack_from('<I', ino, 20)
            ti.linkname = ino[24: 24 + size].decode('utf-8', 'surrogateescape')
        elif t in (4, 5, 11, 12):
            rdev, = struct.unpack_from('<I', ino, 20)
            ti.devmajor, ti.devminor = (rdev >> 8) & 0xFFF, (rdev & 0xFF) | ((rdev >> 12) & 0xFFF00)
        tar.addfile(ti, io.BytesIO(data) if data is not None else None)
        count += 1
    return count


def tar_directory(src, tar):
    count = 0

    def as_root(ti):
        nonlocal count
        if stat.S_ISSOCK(os.lstat(os.path.join(src, ti.name)).st_mode):
            return None
        ti.uid = ti.gid = 0
        ti.uname = ti.gname = ''
        count += 1
        return ti
    tar.add(src, arcname='.', recursive=True, filter=as_root)
    return count


def main():
    if len(sys.argv) not in (2, 3):
        raise SystemExit(__doc__)
    src = sys.argv[1]
    out = open(sys.argv[2], 'wb') if len(sys.argv) == 3 else sys.stdout.buffer
    with tarfile.open(fileobj=out, mode='w|', format=tarfile.GNU_FORMAT) as tar:
        n = tar_directory(src, tar) if os.path.isdir(src) else tar_squashfs(src, tar)
    print('sqfs2tar: %d entries from %s' % (n, src), file=sys.stderr)


if __name__ == '__main__':
    main()
