#!/usr/bin/python3

import os
import sys
import time

BLKSIZE = 1024
SZINODE = 32
NDIRECT = 10

roundup = lambda x,y : (x // y + (1 if x % y else 0)) * y
rounddown = lambda x,y : x - x % y

boot = "boot/boot"
apps = ["kernel/kernel"]
uapp = ["init", "sh", "cat", "cp", "echo", "ln", "ls", "mkdir", "mv", "rm", "rmdir"]
uapp+= ["ab"]
uapp = ["user/"+i for i in uapp]
apps+= uapp
apps+= ["LICENSE", "README.md"]

nzones	= 8192	# 0x2000
ninodes	= 256	# 0x100
ibmb	= 2		# 0x02
zbmb	= 3		# 0x03
ibb		= 4		# 0x04
zbb		= 12	# 0x0c
magic	= 0xcafe
rooti	= 1

fblk = bytearray(BLKSIZE)
for i in range(0, BLKSIZE):
	fblk[i] = 0xFF

sb = [nzones, ninodes, ibmb, zbmb, ibb, zbb, magic, rooti]
sbinfo = bytes(0)
for i in sb:
	sbinfo += int.to_bytes(i,2,'little')

def ddinode(fs, i, si, t):
	fs.seek(fs.ibb * BLKSIZE + (i - 1) * SZINODE, 0)
	fs.write(t.to_bytes(1, 'little'))
	fs.write(b'\x01')
	a = si.to_bytes(4, 'little')
	fs.write(a)
	a = int(time.time()).to_bytes(4, 'little')
	fs.write(a)
	si = roundup(si, BLKSIZE) // BLKSIZE
	bf = b''
	ti = si
	if si > NDIRECT:
		for i in range(NDIRECT, si):
			bf += (fs.avz + i).to_bytes(2, 'little')
		si = NDIRECT
	for i in range(0, si):
		fs.write((fs.avz+i).to_bytes(2,'little'))
	fs.avz += ti
	if bf != b'':
		fs.write((fs.avz).to_bytes(2,'little'))
		fs.seek((fs.zbb + fs.avz - 1) * BLKSIZE, 0)
		fs.write(bf)
		fs.avz += 1


def dd2fs(fs, p):
	fp = open(p, 'rb')
	bf = fp.read(-1)
	
	fs.seek((fs.avz - 1 + fs.zbb) * BLKSIZE, 0)
	fs.write(bf)

	ddinode(fs, fs.avi, len(bf), 0)

	fs.avi += 1

def fsinit(nm, apps):
	def fsinitroot(fs, apps):
		ddinode(fs, 1, 1024, 1)
		fs.avi += 1
		fs.seek(fs.zbb * BLKSIZE, 0)
		fs.write(b'\x01\x00.' + bytes(13))
		fs.write(b'\x01\x00..' + bytes(12))
		for i in range(0, len(apps)):
			bf = (2+i).to_bytes(2,'little')
			fn = os.path.basename(apps[i]).encode('utf-8')
			pd = bytes(14 - len(fn))
			fs.write(bf + fn + pd)

	fs = open(nm, 'wb+')
	fs.ninodes = 256
	fs.ibmb = 2
	fs.zbmb = 3
	fs.ibb = 4
	fs.zbb  =12
	fs.avi = 1
	fs.avz = 1
	fs.write(bytes(8210 * BLKSIZE))
	
	fs.seek(0,0)
	fs.write(open(boot, 'rb').read(-1))
	
	fs.seek(BLKSIZE, 0)
	fs.write(sbinfo)

	fs.seek(fs.ibmb * BLKSIZE, 0)
	fs.write(fblk)
	fs.seek(fs.ibmb * BLKSIZE, 0)
	fs.write(bytes(fs.ninodes//8))
	
	fsinitroot(fs, apps)
	return fs

def fsupdatebm(fs):
	fs.seek(fs.ibmb * BLKSIZE, 0)
	nbi = fs.avi - 1
	nbp = roundup(nbi, 8)
	ibm = (2 ** nbi - 1).to_bytes(nbp // 8, 'little')
	fs.write(ibm)
	fs.seek(fs.zbmb * BLKSIZE, 0)
	nbi = fs.avz - 1
	nbp = roundup(nbi, 8)
	zbm = (2 ** nbi - 1).to_bytes(nbp // 8, 'little')
	fs.write(zbm)

fs = fsinit(sys.argv[1], apps)

for pa in apps:
	dd2fs(fs, pa)

fsupdatebm(fs)

fs.flush()
fs.close()
