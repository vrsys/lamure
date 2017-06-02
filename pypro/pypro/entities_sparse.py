from .entities_common import *


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
        out_bytes.extend(struct.pack("d", self.focal_length))
        out_bytes.extend(self.quat.out())
        out_bytes.extend(self.center.out())
        out_bytes.extend(len(bytearray(self.file_path, encoding='UTF-8')).to_bytes(2, byteorder='big'))
        out_bytes.extend(bytearray(self.file_path, encoding='UTF-8'))
        out_bytes.extend(self.meta_data.out())
        return out_bytes


class SparsePoint(Point):
    def __init__(self, index, position, color, meta_data, measurements):
        super(SparsePoint, self).__init__(index, position, color, meta_data)
        self.measurements = measurements

    def out(self):
        out_bytes = bytearray()
        out_bytes.extend(self.index.to_bytes(4, byteorder='big'))
        out_bytes.extend(self.position.out())
        out_bytes.extend(self.color.out())
        out_bytes.extend(len(self.measurements).to_bytes(2, byteorder='big'))
        for measurement in self.measurements:
            out_bytes.extend(measurement.out())
        out_bytes.extend(self.meta_data.out())
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
