"""Exercise the actual resource parser built as a host DLL by test.ps1."""
from pathlib import Path
import ctypes as C
import io
import os
import sys
import tempfile
import xml.etree.ElementTree as ET

root = Path(__file__).resolve().parents[2]
lib = C.CDLL(str(Path(sys.argv[1]).resolve()))
start_type = C.CFUNCTYPE(None, C.c_void_p, C.c_char_p, C.POINTER(C.c_char_p))
end_type = C.CFUNCTYPE(None, C.c_void_p, C.c_char_p)
lib.XML_ParserCreate.argtypes = [C.c_char_p]
lib.XML_ParserCreate.restype = C.c_void_p
lib.XML_SetElementHandler.argtypes = [C.c_void_p, start_type, end_type]
lib.XML_Parse.argtypes = [C.c_void_p, C.c_char_p, C.c_int, C.c_int]
lib.XML_Parse.restype = C.c_int
lib.XML_ParserFree.argtypes = [C.c_void_p]
lib.ZumaTestFormat.restype = C.c_int
assert lib.ZumaTestFormat() == 0
print("PASS: score/timer formatting, padding and decimal output")

def parse(data, chunk):
    events = []
    @start_type
    def start(_, name, attributes):
        pairs = {}
        i = 0
        while attributes[i]:
            pairs[attributes[i].decode()] = attributes[i+1].decode()
            i += 2
        events.append(("start", name.decode(), pairs))
    @end_type
    def end(_, name):
        events.append(("end", name.decode(), {}))
    p = lib.XML_ParserCreate(None)
    assert p
    try:
        lib.XML_SetElementHandler(p, start, end)
        for pos in range(0, len(data), chunk):
            part = data[pos:pos+chunk]
            assert lib.XML_Parse(p, part, len(part), 0)
        ok = bool(lib.XML_Parse(p, b"", 0, 1))
        return ok, events
    finally:
        lib.XML_ParserFree(p)

data = (root / "content/levels/levels.xml").read_bytes()
expected = [(kind, element.tag, dict(element.attrib) if kind == "start" else {})
            for kind, element in ET.iterparse(io.BytesIO(data), events=("start", "end"))]
for chunk in (1, 7, 32768):
    ok, events = parse(data, chunk)
    assert ok and events == expected, f"Resource parse mismatch, chunk={chunk}"
ok, events = parse(b'<r a="&amp;&lt;&gt;&quot;&apos;"><c/></r>', 3)
assert ok and events[0][2]["a"] == '&<>"\''
for invalid in (b"", b"<r>", b"<r></q>", b"<r a=x/>", b"<!DOCTYPE r><r/>",
                b"<r/><!--", b"<r/><s/>", b'<r a="&unknown;"/>'):
    assert not parse(invalid, 2)[0], invalid
print(f"PASS: resource parser matches ElementTree ({len(expected)} events), streaming, entities, malformed input")

# Isolate saves from the user's real progress and the prepared boot directory.
with tempfile.TemporaryDirectory(prefix="zuma-levels-", dir=Path(sys.argv[1]).resolve().parent) as tmp:
    directory = Path(tmp)
    (directory / "levels").mkdir()
    (directory / "levels/levels.xml").write_bytes(data)
    previous = Path.cwd()
    try:
        os.chdir(directory)
        lib.ZumaTestLevels.restype = C.c_int
        result = lib.ZumaTestLevels()
        assert result == 0, f"Level loader or save round trip failed: {result}"
    finally:
        os.chdir(previous)
print("PASS: actual level loader, 18 playable levels, score/time save round trip")
