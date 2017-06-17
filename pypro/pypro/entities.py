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

    def out(self):
        out_bytes = bytearray()
        # out_bytes.extend(self.index.to_bytes(4, byteorder='big'))
        out_bytes.extend(self.position.out())
        out_bytes.extend(self.color.out())
        return out_bytes


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
        # out_bytes.extend(len(self.charbuff).to_bytes(4, byteorder='big'))
        out_bytes.extend(self.charbuff)
        return out_bytes


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
        out_bytes.extend(super(DensePoint, self).out())
        out_bytes.extend(self.normal.out())
        # out_bytes.extend(self.meta_data.out())
        return out_bytes


class Camera(Entity):
    def __init__(self, index, focal_length, quat, center, file_path, meta_data):
        self.index = index
        self.focal_length = focal_length
        self.quat = quat
        self.center = center
        self.file_path = file_path
        self.meta_data = meta_data

    def out(self):
        out_bytes = bytearray()
        out_bytes.extend(self.index.to_bytes(2, byteorder='big'))
        out_bytes.extend(struct.pack(">d", self.focal_length))
        out_bytes.extend(self.quat.out())
        out_bytes.extend(self.center.out())
        # out_bytes.extend(len(bytearray(self.file_path, encoding='UTF-8')).to_bytes(2, byteorder='big'))
        # out_bytes.extend(bytearray(self.file_path, encoding='UTF-8'))
        # out_bytes.extend(self.meta_data.out())
        return out_bytes


class SparsePoint(Point):
    def __init__(self, index, position, color, meta_data, measurements):
        super(SparsePoint, self).__init__(index, position, color, meta_data)
        self.measurements = measurements

    def out(self):
        out_bytes = bytearray()
        out_bytes.extend(self.index.to_bytes(4, byteorder='big'))
        out_bytes.extend(super(SparsePoint, self).out())
        out_bytes.extend(len(self.measurements).to_bytes(2, byteorder='big'))
        for measurement in self.measurements:
            out_bytes.extend(measurement.out())
        # out_bytes.extend(self.meta_data.out())
        return out_bytes


class Measurement(Entity):
    def __init__(self, camera_index, occurence_x, occurence_y):
        self.camera_index = camera_index
        self.occurence_x = occurence_x
        self.occurence_y = occurence_y

    def out(self):
        out_bytes = bytearray()
        out_bytes.extend(self.camera_index.to_bytes(2, byteorder='big'))
        out_bytes.extend(struct.pack(">d", self.occurence_x))
        out_bytes.extend(struct.pack(">d", self.occurence_y))
        return out_bytes
