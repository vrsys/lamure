import zlib


class Format(object):
    def out(self):
        pass


class FormatSparse(Format):
    cameras = []
    spoints = []


class FormatDense(Format):
    dpoints = []


class FormatSparsePro(FormatSparse):
    magic_bytes = bytes([0xAF, 0xFE])

    def out(self):
        # TODO: test, ensure compliance
        out_bytes = bytearray()

        out_bytes.extend(len(self.cameras).to_bytes(2, byteorder='big'))

        for camera in self.cameras:
            out_bytes.extend(camera.out())

        out_bytes.extend(len(self.spoints).to_bytes(4, byteorder='big'))

        for point in self.spoints:
            out_bytes.extend(point.out())

        out_file = open('sparse.prov', 'wb')
        out_file.write(self.magic_bytes)
        out_file.write((zlib.crc32(out_bytes) & 0xffffffff).to_bytes(4, byteorder='big'))
        out_file.write(len(out_bytes).to_bytes(4, byteorder='big'))
        out_file.write(out_bytes)
        out_file.close()


class FormatDensePro(FormatDense):
    magic_bytes = bytes([0xAF, 0xFE])

    def out(self):
        # TODO: test, ensure compliance
        out_bytes = bytearray()

        out_bytes.extend(len(self.dpoints).to_bytes(4, byteorder='big'))

        for point in self.dpoints:
            out_bytes.extend(point.out())

        out_file = open('dense.prov', 'wb')
        out_file.write(self.magic_bytes)
        out_file.write((zlib.crc32(out_bytes) & 0xffffffff).to_bytes(4, byteorder='big'))
        out_file.write(len(out_bytes).to_bytes(4, byteorder='big'))
        out_file.write(out_bytes)
        out_file.close()
