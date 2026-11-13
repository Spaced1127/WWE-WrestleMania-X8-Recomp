import struct, re, sys

PATH = sys.argv[1] if len(sys.argv) > 1 else r'H:\Projects\SB-RFD\work\disc\files\DATA\BB\LVL_BBSH.DGC'
d = open(PATH, 'rb').read()

def rd_obj(pos, depth=0):
    if depth > 40:
        raise ValueError('deep')
    t = d[pos]; pos += 1
    if t in (ord('s'), ord('t')):
        n = struct.unpack_from('<i', d, pos)[0]; pos += 4
        return d[pos:pos+n], pos + n
    if t == ord('N'): return None, pos
    if t == ord('T'): return True, pos
    if t == ord('F'): return False, pos
    if t == ord('i'): return struct.unpack_from('<i', d, pos)[0], pos + 4
    if t == ord('l'): return struct.unpack_from('<q', d, pos)[0], pos + 8
    if t == ord('f'): return struct.unpack_from('<d', d, pos)[0], pos + 8
    if t == ord('x'): return complex(*struct.unpack_from('<dd', d, pos)), pos + 16
    if t == ord('(') or t == ord('['):
        n = struct.unpack_from('<i', d, pos)[0]; pos += 4
        out = []
        for _ in range(n):
            v, pos = rd_obj(pos, depth + 1); out.append(v)
        return out, pos
    if t == ord('{'):
        out = {}
        while True:
            if d[pos] == ord('0'):
                pos += 1
                return out, pos
            k, pos = rd_obj(pos, depth + 1)
            v, pos = rd_obj(pos, depth + 1)
            out[k] = v
    if t == ord('c'):
        return rd_code(pos - 1, depth + 1)
    raise ValueError('type %02x at %x' % (t, pos - 1))

def rd_code(pos, depth=0):
    assert d[pos] == ord('c'); pos += 1
    ac, nl, ss, fl = struct.unpack_from('<4H', d, pos); pos += 8
    code, pos = rd_obj(pos, depth + 1)
    consts, pos = rd_obj(pos, depth + 1)
    names, pos = rd_obj(pos, depth + 1)
    varnames, pos = rd_obj(pos, depth + 1)
    freevars, pos = rd_obj(pos, depth + 1)
    cellvars, pos = rd_obj(pos, depth + 1)
    filename, pos = rd_obj(pos, depth + 1)
    name, pos = rd_obj(pos, depth + 1)
    lineno = struct.unpack_from('<H', d, pos)[0]; pos += 2
    lnotab, pos = rd_obj(pos, depth + 1)
    if not isinstance(code, (bytes, bytearray)) or not isinstance(consts, list) or not isinstance(names, list):
        raise ValueError('shape')
    return {'hdr': (ac, nl, ss, fl), 'code': bytes(code), 'consts': consts,
            'names': names, 'varnames': varnames, 'filename': filename,
            'name': name, 'lineno': lineno}, pos

seen = set()
classes = []
for m in re.finditer(rb'c', d):
    try:
        c, _ = rd_code(m.start())
    except Exception:
        continue
    key = (m.start(),)
    if c['filename'] != b'LVL_BBSH':
        continue
    if c['name'] in (b'', b'?'):
        continue
    # class body: consts contain only code objects/None and name is uppercase-ish
    nested = [x for x in c['consts'] if isinstance(x, dict)]
    if nested and all(isinstance(x, dict) for x in c['consts'] if x is not None):
        classes.append((m.start(), c))

print('classes', len(classes))
for p, c in classes:
    print('==', hex(p), c['name'])
    for k in c['consts']:
        if isinstance(k, dict):
            strs = [x for x in k['consts'] if isinstance(x, (bytes, bytearray))]
            print('   ', k['name'], 'names=', [x for x in k['names'] if isinstance(x, (bytes, bytearray))][:8],
                  'consts=', [x[:28] for x in strs][:8])
