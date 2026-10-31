Import("env")
from pathlib import Path


def merge(source, target, env):
    build = Path(env.subst("$BUILD_DIR"))
    parts = [(0, "bootloader.bin"), (0x8000, "partitions.bin"), (0x10000, "firmware.bin")]
    image = bytearray()
    for offset, name in parts:
        data = (build / name).read_bytes()
        if len(image) > offset:
            raise RuntimeError("Overlapping flash images")
        image.extend(b"\xff" * (offset - len(image)))
        image.extend(data)
    output = build / "tdeck-sd-maintenance-v1.bin"
    output.write_bytes(image)
    print("Merged maintenance firmware:", output)


env.AddPostAction("$BUILD_DIR/firmware.bin", merge)
