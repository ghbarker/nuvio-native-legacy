"""Read actual declared APK DEX method prototypes and access flags."""
import hashlib,struct,zlib,zipfile

def declared_methods(data,classname):
 assert data[:4]==b'dex\n' and data[7]==0
 u16=lambda p:struct.unpack_from('<H',data,p)[0]
 u32=lambda p:struct.unpack_from('<I',data,p)[0]
 assert u32(32)==len(data) and u32(40)==0x12345678
 assert u32(8)==zlib.adler32(data[12:]) & 0xffffffff
 assert data[12:32]==hashlib.sha1(data[32:]).digest()
 def leb(pos):
  value=shift=0
  for _ in range(5):
   b=data[pos];pos+=1;value|=(b&127)<<shift
   if b<128:return value,pos
   shift+=7
  raise AssertionError('Invalid ULEB128')
 strings=[]
 for i in range(u32(56)):
  pos=u32(u32(60)+4*i);_,pos=leb(pos);end=data.index(0,pos)
  # JNI names and descriptors are ASCII. Unrelated UI MUTF8 strings can
  # contain Java's overlong NUL or surrogate pairs; they are never matched.
  strings.append(data[pos:end].decode('utf-8',errors='replace'))
 types=[strings[u32(u32(68)+4*i)] for i in range(u32(64))]
 protos=[]
 for i in range(u32(72)):
  pos=u32(76)+12*i;ret=types[u32(pos+4)];params=u32(pos+8)
  args=[] if params==0 else [types[u16(params+4+2*j)] for j in range(u32(params))]
  protos.append('('+''.join(args)+')'+ret)
 ids=[]
 for i in range(u32(88)):
  pos=u32(92)+8*i;ids.append((types[u16(pos)],strings[u32(pos+4)],protos[u16(pos+2)]))
 result={}
 for i in range(u32(96)):
  pos=u32(100)+32*i
  if types[u32(pos)]!=classname:continue
  offset=u32(pos+24);assert offset
  sf,offset=leb(offset);inf,offset=leb(offset);direct,offset=leb(offset);virtual,offset=leb(offset)
  for _ in range(sf+inf):_,offset=leb(offset);_,offset=leb(offset)
  for size in (direct,virtual):
   index=0
   for _ in range(size):
    diff,offset=leb(offset);index+=diff;flags,offset=leb(offset);_,offset=leb(offset)
    owner,name,descriptor=ids[index];assert owner==classname
    result[name,descriptor]=flags
 return result

def apk_methods(apk,classname):
 result={};digests={}
 with zipfile.ZipFile(apk) as z:
  for name in z.namelist():
   if name.startswith('classes') and name.endswith('.dex') and '/' not in name:
    data=z.read(name);digests[name]=hashlib.sha256(data).hexdigest()
    methods=declared_methods(data,classname)
    assert not (result.keys() & methods.keys()), 'Duplicate class/method definition in APK'
    result.update(methods)
 assert digests, 'APK DEX absent'
 return result,digests
