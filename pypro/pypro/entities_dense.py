import struct

from .entities_common import Entity, Point


class Normal(Entity):
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


class DensePoint(Point):
    def __init__(self, index, position, color, meta_data, normal):
        super(DensePoint, self).__init__(index, position, color, meta_data)
        self.normal = normal

    def out(self):
        out_bytes = bytearray()
        out_bytes.extend(self.index.to_bytes(4, byteorder='big'))
        out_bytes.extend(self.position.out())
        out_bytes.extend(self.color.out())
        out_bytes.extend(self.normal.out())
        out_bytes.extend(self.meta_data.out())
        return out_bytes
