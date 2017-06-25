class Format(object):
    def out(self):
        pass


class FormatSparsePro(Format):
    spoints = []
    cameras = []
    magic_bytes = bytes([0xAF, 0xFE])
    length_meta_data_camera = 0
    length_max_file_path = 0
    length_meta_data_spoints = 0

    def out(self):
        # TODO: test, ensure compliance
        out_bytes_prov = bytearray()
        out_bytes_meta = bytearray()

        out_bytes_prov.extend(len(self.spoints).to_bytes(4, byteorder='big'))
        out_bytes_prov.extend(self.length_meta_data_spoints.to_bytes(4, byteorder='big'))

        for point in self.spoints:
            out_bytes_prov.extend(point.out())
            out_bytes_meta.extend(point.meta_data.out())

        out_bytes_prov.extend(len(self.cameras).to_bytes(2, byteorder='big'))
        out_bytes_prov.extend(self.length_meta_data_camera.to_bytes(4, byteorder='big'))
        out_bytes_prov.extend(self.length_max_file_path.to_bytes(2, byteorder='big'))

        for camera in self.cameras:
            out_bytes_prov.extend(camera.out())
            camera.file_path += ' ' * (self.length_max_file_path - len(bytearray(camera.file_path, encoding='UTF-8')))
            out_bytes_prov.extend(bytearray(camera.file_path, encoding='UTF-8'))
            out_bytes_meta.extend(camera.meta_data.out())

        out_file = open('sparse.prov', 'wb')
        out_file.write(self.magic_bytes)
        out_file.write(len(out_bytes_prov).to_bytes(8, byteorder='big'))
        out_file.write(out_bytes_prov)
        out_file.close()

        out_file = open('sparse.prov.meta', 'wb')
        out_file.write(self.magic_bytes)
        out_file.write(len(out_bytes_meta).to_bytes(8, byteorder='big'))
        out_file.write(out_bytes_meta)
        out_file.close()


class FormatDensePro(Format):
    dpoints = []
    magic_bytes = bytes([0xAF, 0xFE])
    length_meta_data_dpoints = 0

    def out(self):
        # TODO: test, ensure compliance
        out_bytes = bytearray()
        out_bytes_meta = bytearray()

        out_bytes.extend(len(self.dpoints).to_bytes(4, byteorder='big'))
        out_bytes.extend(self.length_meta_data_dpoints.to_bytes(4, byteorder='big'))

        for point in self.dpoints:
            out_bytes.extend(point.out())
            out_bytes_meta.extend(point.meta_data.out())

        out_file = open('dense.prov', 'wb')
        out_file.write(self.magic_bytes)
        out_file.write(len(out_bytes).to_bytes(8, byteorder='big'))
        out_file.write(out_bytes)
        out_file.close()

        out_file = open('dense.prov.meta', 'wb')
        out_file.write(self.magic_bytes)
        out_file.write(len(out_bytes_meta).to_bytes(8, byteorder='big'))
        out_file.write(out_bytes_meta)
        out_file.close()
