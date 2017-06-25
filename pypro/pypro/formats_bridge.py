from .entities import *
from .formats import *


class FormatSparseNVMV3(FormatSparsePro):

    def __init__(self, path_to_nvm_file):
        self.file_path_nvm = path_to_nvm_file
        self.length_meta_data_camera = 0
        self.length_max_file_path = 0x16
        self.length_meta_data_spoints = 0
        self.init()

    def init(self):
        in_file = open(self.file_path_nvm, mode='r')
        token = in_file.readline().strip()
        if token != "NVM_V3":
            raise ValueError('File ' + self.file_path_nvm + '; format is corrupted')
        in_file.readline()
        ncam = int(in_file.readline().strip())

        for i in range(0, ncam):
            line = in_file.readline()
            line_values = line.split()
            file_path = str(line_values[0].strip())
            focal_length = float(line_values[1].strip())
            quat = Quat(float(line_values[2].strip()),
                        float(line_values[3].strip()),
                        float(line_values[4].strip()),
                        float(line_values[5].strip()))
            center = Position(float(line_values[6].strip()),
                              float(line_values[7].strip()),
                              float(line_values[8].strip()))
            charbuff = bytearray()
            charbuff.extend(struct.pack(">d", float(line_values[9].strip())))
            charbuff.extend(struct.pack(">d", float(line_values[10].strip())))
            meta_data = MetaData(charbuff=charbuff)
            self.cameras.append(
                Camera(index=i,
                       focal_length=focal_length,
                       quat=quat,
                       center=center,
                       meta_data=meta_data,
                       file_path=file_path)
            )

        in_file.readline()
        npoint = int(in_file.readline().strip())

        for i in range(0, npoint):
            line = in_file.readline()
            line_values = line.split()
            position = Position(float(line_values[0].strip()),
                                float(line_values[1].strip()),
                                float(line_values[2].strip()))
            color = Color(float(line_values[3].strip()),
                          float(line_values[4].strip()),
                          float(line_values[5].strip()))
            nmeasurement = int(line_values[6].strip())
            measurements = []
            for k in range(0, nmeasurement):
                measurements.append(Measurement(
                    camera_index=int(line_values[6 + 1 + k * 4].strip()),
                    occurence_x=float(line_values[6 + 3 + k * 4].strip()),
                    occurence_y=float(line_values[6 + 4 + k * 4].strip())))
            charbuff = bytearray()
            meta_data = MetaData(charbuff=charbuff)
            self.spoints.append(SparsePoint(
                index=i,
                position=position,
                color=color,
                meta_data=meta_data,
                measurements=measurements))

        in_file.close()


class FormatDensePMVS(FormatDensePro):

    def __init__(self, path_to_patch, path_to_ply):
        self.file_path_patch = path_to_patch
        self.file_path_ply = path_to_ply
        self.length_meta_data_dpoints = 0
        self.init()

    def init(self):
        in_file_patch = open(self.file_path_patch, mode='r')
        in_file_ply = open(self.file_path_ply, mode='r')

        token = in_file_patch.readline().strip()
        if token != "PATCHES":
            raise ValueError('File ' + self.file_path_patch + '; format is corrupted')

        token = in_file_ply.readline().strip()
        if token != "ply":
            raise ValueError('File ' + self.file_path_patch + '; format is corrupted')

        for i in range(0, 12):  # HEADERS
            in_file_ply.readline()

        npoint = int(in_file_patch.readline().strip())

        for i in range(0, npoint):
            if i % round(npoint / 20) == 0:
                print('Reading PMVS/CMVS: ' + str(round(i * 100 / npoint)) + '%')

            in_file_patch.readline()  # PATCHS

            line = in_file_patch.readline()
            line_values = line.split()
            position = Position(float(line_values[0].strip()),
                                float(line_values[1].strip()),
                                float(line_values[2].strip()))

            line = in_file_patch.readline()
            line_values = line.split()
            normal = Normal(float(line_values[0].strip()),
                            float(line_values[1].strip()),
                            float(line_values[2].strip()))

            line = in_file_patch.readline()
            line_values = line.split()
            confidence = float(line_values[0].strip())
            charbuff = bytearray()
            charbuff.extend(struct.pack(">d", confidence))
            meta_data = MetaData(charbuff=charbuff)

            line = in_file_ply.readline()
            line_values = line.split()
            color = Color(float(line_values[6].strip()),
                          float(line_values[7].strip()),
                          float(line_values[8].strip()))

            self.dpoints.append(DensePoint(
                index=i,
                position=position,
                color=color,
                meta_data=meta_data,
                normal=normal))

            for i in range(0, 5):  # Relationships
                in_file_patch.readline()

        in_file_patch.close()
        in_file_ply.close()
