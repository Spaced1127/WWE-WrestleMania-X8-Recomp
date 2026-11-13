import struct, sys, re

PATH = r'H:\Projects\SB-RFD\work\disc\files\DATA\BB\LVL_BBSH.DGC'
data = open(PATH, 'rb').read()

CODE = ord('c')
STRING = ord('s')
INTERN = ord('t')
TUPLE = ord('(')
LIST = ord('[')
DICT = ord('{')
INT = ord('i')
NONE = ord('N')
TRUE = ord('T')
FALSE = ord('F')
FLOAT = ord('f')

class Reader:
    def __init__(self, d, p):
        self.d = d
        self.p = p
    def u8(self):
        v = self.d[self.p]; self.p += 1; return v
    def i32(self):
        v = struct.unpack_from('<i', self.d, self.p)[0]; self.p += 4; return v
    def take(self, n):
        v = self.d[self.p:self.p+n]; self.p += n; return v

def read_object(r, depth=0):
    if depth > 30:
        raise ValueError('deep')
    t = r.u8()
    if t == STRING or t == INTERN:
        n = r.i32()
        if n < 0 or n > 2_000_000:
            raise ValueError('len')
        return r.take(n)
    if t == NONE:
        return None
    if t == TRUE:
        return True
    if t == FALSE:
        return False
    if t == INT:
        return r.i32()
    if t == FLOAT:
        return struct.unpack('<d', r.take(8))[0]
    if t == TUPLE:
        n = r.i32()
        if n < 0 or n > 10000:
            raise ValueError('tuple')
        return tuple(read_object(r, depth+1) for _ in range(n))
    if t == LIST:
        n = r.i32()
        if n < 0 or n > 10000:
            raise ValueError('list')
        return [read_object(r, depth+1) for _ in range(n)]
    if t == DICT:
        out = {}
        while True:
            k = read_object(r, depth+1)
            if k is None and r.d[r.p] == 0:
                r.p += 1
                return out
            out[k] = read_object(r, depth+1)
    if t == CODE:
        r.p -= 1
        return read_code(r, depth+1)
    raise ValueError('type %r' % chr(t) if 32 <= t < 127 else 'type %02x' % t)

def read_code(r, depth=0):
    r.u8()  # 'c'
    argcount = r.i32()
    nlocals = r.i32()
    stacksize = r.i32()
    flags = r.i32()
    if not (0 <= argcount < 10 and 0 <= nlocals < 64 and 0 <= stacksize < 400):
        raise ValueError('hdr')
    code = read_object(r, depth+1)
    consts = read_object(r, depth+1)
    names = read_object(r, depth+1)
    varnames = read_object(r, depth+1)
    freevars = read_object(r, depth+1)
    cellvars = read_object(r, depth+1)
    filename = read_object(r, depth+1)
    name = read_object(r, depth+1)
    firstlineno = r.i32()
    lnotab = read_object(r, depth+1)
    if not isinstance(code, (bytes, bytearray)) or not isinstance(consts, tuple) or not isinstance(names, tuple):
        raise ValueError('shape')
    return {
        'argcount': argcount, 'nlocals': nlocals, 'stacksize': stacksize,
        'flags': flags, 'code': bytes(code), 'consts': consts, 'names': names,
        'varnames': varnames, 'filename': filename, 'name': name,
        'firstlineno': firstlineno,
    }

def walk(c, out, depth=0):
    out.append(c)
    for k in c['consts']:
        if isinstance(k, dict) and 'code' in k:
            walk(k, out, depth+1)

found = []
for m in re.finditer(rb'c', data):
    p = m.start()
    try:
        r = Reader(data, p)
        c = read_code(r)
        text = repr(c['consts']) + repr(c['names'])
        if 'ScriptLoadLevel' in text or 'VOL_TELEPORT' in text or 'BACKDOOR' in text:
            found.append((p, c))
    except Exception:
        continue

print('candidates', len(found))
for p, c in found[:20]:
    print('==', hex(p), 'name', c['name'], 'file', c['filename'], 'nconst', len(c['consts']))
    print('  consts:', [x if not isinstance(x, bytes) else x[:60] for x in c['consts']][:20])
    print('  names:', [x if not isinstance(x, bytes) else x[:60] for x in c['names']][:30])
