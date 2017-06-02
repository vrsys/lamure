import struct


class Entity(object):
    def out(self):
        return bytes([0xAA, 0xAA])


class Point(Entity):
    def __init__(self, index, position, color, meta_data):
        self.index = index
        self.position = position
        self.color = color
        self.meta_data = meta_data


class Position(Entity):
    def __init__(self, x, y, z):
        self.x = x
        self.y = y
        self.z = z

    def out(self):
        out_bytes = bytearray()
        out_bytes.extend(struct.pack(">d", self.x))
        out_bytes.extend(struct.pack(">d", self.y))
        out_bytes.extend(struct.pack(">d", self.z))
        return out_bytes


class Color(Entity):
    def __init__(self, r, g, b):
        self.r = r
        self.g = g
        self.b = b

    def out(self):
        out_bytes = bytearray()
        out_bytes.extend(struct.pack(">d", self.r))
        out_bytes.extend(struct.pack(">d", self.g))
        out_bytes.extend(struct.pack(">d", self.b))
        return out_bytes


class Quat(Entity):
    def __init__(self, w, x, y, z):
        self.w = w
        self.x = x
        self.y = y
        self.z = z

    def out(self):
        out_bytes = bytearray()
        out_bytes.extend(struct.pack(">d", self.w))
        out_bytes.extend(struct.pack(">d", self.x))
        out_bytes.extend(struct.pack(">d", self.y))
        out_bytes.extend(struct.pack(">d", self.z))
        return out_bytes


class MetaData(Entity):
    def __init__(self, charbuff):
        self.charbuff = charbuff

    def out(self):
        out_bytes = bytearray()
        out_bytes.extend(len(self.charbuff).to_bytes(4, byteorder='big'))
        out_bytes.extend(self.charbuff)
        return out_bytes
