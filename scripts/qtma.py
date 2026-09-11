"""Bounded reader for the bundled QuickTime music tracks (QuickTimeMusic.h)."""
import struct


def u32(data, offset=0):
    return struct.unpack_from(">I", data, offset)[0]


def atoms(data):
    offset = 0
    while offset < len(data):
        size, kind = struct.unpack_from(">I4s", data, offset)
        if size < 8 or offset + size > len(data):
            raise ValueError("invalid atom size")
        yield kind, data[offset + 8:offset + size]
        offset += size


def child(data, kind):
    matches = [body for name, body in atoms(data) if name == kind]
    if len(matches) != 1:
        raise ValueError(f"expected one {kind!r}")
    return matches[0]


def events(data):
    offset = 0
    while offset < len(data):
        word = u32(data, offset)
        kind = word >> 29 if word >> 29 < 4 else word >> 28
        size = 4 if kind < 4 else (4 * (word & 65535) if kind == 15 else 8)
        if size < 4 or offset + size > len(data):
            raise ValueError("invalid QTMA event size")
        yield kind, word, data[offset:offset + size]
        offset += size


def music(data):
    media = child(child(child(data, b"moov"), b"trak"), b"mdia")
    mdhd = child(media, b"mdhd")
    if mdhd[0] != 0:
        raise ValueError("unsupported media header")
    scale = u32(mdhd, 12)
    table = child(child(media, b"minf"), b"stbl")
    stsd = child(table, b"stsd")
    if u32(stsd, 4) != 1:
        raise ValueError("multiple sample descriptions")
    header = child(stsd[8:], b"musi")[12:]
    stsz = child(table, b"stsz")
    fixed, count = struct.unpack_from(">II", stsz, 4)
    sizes = [fixed] * count if fixed else struct.unpack_from(f">{count}I", stsz, 12)
    stco = child(table, b"stco")
    offsets = struct.unpack_from(f">{u32(stco, 4)}I", stco, 8)
    stsc = child(table, b"stsc")
    entries = [struct.unpack_from(">III", stsc, 8 + i * 12) for i in range(u32(stsc, 4))]
    samples, index = [], 0
    for chunk, offset in enumerate(offsets, 1):
        mapping = [entry for entry in entries if entry[0] <= chunk]
        if not mapping or mapping[-1][2] != 1:
            raise ValueError("invalid sample mapping")
        for _ in range(mapping[-1][1]):
            size = sizes[index]
            if offset + size > len(data):
                raise ValueError("truncated sample")
            samples.append(data[offset:offset + size])
            offset += size
            index += 1
    if index != count or not 0 < scale < 32768:
        raise ValueError("invalid sample count or timescale")
    return header, b"".join(samples), scale


def performance(data):
    header, sequence, scale = music(data)
    channels, programs = {}, {}
    melodic = iter(i for i in range(16) if i != 9)
    for kind, word, payload in events(header):
        if kind == 15 and (u32(payload, len(payload) - 4) >> 16) & 16383 == 1:
            part, gm = (word >> 16) & 4095, u32(payload, 84)
            drums = gm >= 0x4000
            program = gm - (0x4001 if drums else 1)
            if not 0 <= program < 128:
                raise ValueError(f"unsupported GM instrument {gm}")
            channels[part] = 9 if drums else next(melodic)
            programs[part] = program
    output = [(0, -1, bytes([0xC0 | channel, programs[part]])) for part, channel in channels.items()]
    now = 0
    for kind, word, payload in events(sequence):
        if kind == 0:
            now += word & 0xFFFFFF
        elif kind in (1, 9):
            part = (word >> (24 if kind == 1 else 16)) & (31 if kind == 1 else 4095)
            if kind == 1:
                pitch, velocity, duration = ((word >> 18) & 63) + 32, (word >> 11) & 127, word & 2047
            else:
                tail = u32(payload, 4)
                pitch, velocity, duration = word & 65535, (tail >> 22) & 127, tail & 0x3FFFFF
            if pitch > 127:
                raise ValueError("extended pitch needs explicit conversion")
            channel = channels[part]
            output.extend([(now, 1, bytes([0x90 | channel, pitch, velocity])),
                           (now + duration, 0, bytes([0x80 | channel, pitch, 0]))])
        elif kind == 2:
            part, control, value = (word >> 24) & 31, (word >> 16) & 255, word & 65535
            if control == 0 and value == 0:
                continue
            if control == 10:
                # QTMA pan: output 1 (left) through output 2 (right), 8.8 fixed.
                value = round((value - 256) * 127 / 256) if value else 64
            elif control in (7, 11, 91, 93):
                value >>= 8
            else:
                raise ValueError(f"unsupported controller {control}")
            output.append((now, 0, bytes([0xB0 | channels[part], control, max(0, min(127, value))])))
        elif kind == 15:
            subtype = (u32(payload, len(payload) - 4) >> 16) & 16383
            # Tune differences contain alternate editing sequences, not main notes.
            if subtype not in (5, 10, 11):
                raise ValueError(f"unsupported general event {subtype}")
        elif kind != 3:
            raise ValueError(f"unsupported event {kind}")
    return sorted(output, key=lambda event: event[:2]), now, scale
