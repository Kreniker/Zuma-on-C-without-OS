"""Check the EFI image and the prepared resource set (Python 3, no packages)."""
from pathlib import Path
import struct
import sys
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[2]
out = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else root / "build-uefi"
boot = out / "esp/EFI/BOOT"
pe = (boot / "BOOTX64.EFI").read_bytes()
u16 = lambda offset: struct.unpack_from("<H", pe, offset)[0]
u32 = lambda offset: struct.unpack_from("<I", pe, offset)[0]
assert pe[:2] == b"MZ"
header = u32(0x3C)
assert pe[header:header+4] == b"PE\0\0"
assert u16(header+4) == 0x8664, "Expected x64"
optional = header + 24
assert u16(optional) == 0x20B, "Expected PE32+"
assert u16(optional+68) == 10, "Expected EFI application subsystem"
assert u32(optional+16), "Missing entry point"
assert u32(optional+112+8) == 0, "Must not import OS DLLs"
assert u32(optional+112+5*8) and u32(optional+112+5*8+4), "Missing relocations"
images = 0
for folder in ("images", "fonts", "levels"):
    for source in (root / "content" / folder).rglob("*"):
        if not source.is_file():
            continue
        target = boot / source.relative_to(root / "content")
        if source.suffix.lower() in (".png", ".jpg", ".jpeg"):
            target = target.with_name(target.name + ".ztex")
            with target.open("rb") as f:
                magic, width, height = struct.unpack("<III", f.read(12))
            assert magic == 0x58544D5A and 0 < width <= 8192 and 0 < height <= 8192, target
            assert target.stat().st_size == 12 + width * height * 4, target
            images += 1
        else:
            assert source.read_bytes() == target.read_bytes(), target
manifest = ET.parse(boot / "levels/levels.xml").getroot()
graphics = {g.attrib["id"]: g for g in manifest.findall("Graphics")}
playable = manifest.find("StageProgression").attrib["stage1"].split(",")
for name in playable:
    g = graphics[name]
    folder = boot / "levels" / g.attrib["id"]
    image = folder / (g.attrib["image"] + ".jpg.ztex")
    assert image.is_file(), image
    curve = folder / (g.attrib["curve"] + ".dat")
    raw = curve.read_bytes()
    count = struct.unpack_from("<I", raw, 0x10)[0]
    offset = 0x14 + count * 10
    points = struct.unpack_from("<I", raw, offset)[0]
    assert points > 1 and offset + 12 + (points - 1) * 4 <= len(raw), curve
    if "image-top" in g.attrib:
        assert (folder / (g.attrib["image-top"] + ".png.ztex")).is_file()
assert len(playable) == 18
assert len(manifest.findall("Settings")) >= 25
print(f"PASS: x64 EFI, no OS imports, relocations; {images} textures; 18 level curves and resources")
