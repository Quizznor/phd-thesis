import struct

# see CDAS git Utils/CL/cl_msg_unknown_pack.h
def read_monit(f) -> dict:

    # convert = lambda d: [
    #     struct.unpack("d", d[i : i + 8])[0]
    #     for i in range(0, len(d), 8)
    # ]
    # package = {}

    # pack Header
    LsId = int.from_bytes(f.read(2), byteorder="little")
    n_packs = int.from_bytes(f.read(2), byteorder="little")
    timestamp_sec = int.from_bytes(f.read(4), byteorder="little")
    timestamp_nsec = int.from_bytes(f.read(4), byteorder="little")
    total_size = int.from_bytes(f.read(4), byteorder="little")
    rv, package = LsId, {}

    for i in range(n_packs):

        # header i
        type = int.from_bytes(f.read(4), byteorder="little")
        version = int.from_bytes(f.read(4), byteorder="little")
        size = int.from_bytes(f.read(4), byteorder="little")
        data = f.read(size)

        package[str(i)] = {
                        'station': LsId,
                        'type': type,
                        'version': version,
                        'size': size,
                        'data': data}

    return package