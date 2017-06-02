from pypro.entities_sparse import *
from .formats import *


class FormatSparseNVMV3(FormatSparse):
    def __init__(self, path_to_nvm_file):
        self.file_path_nvm = path_to_nvm_file
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
