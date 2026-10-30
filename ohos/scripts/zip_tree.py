"""Zip a directory tree with UTF-8 name flags (Chinese paths stay readable).

PowerShell 5.1's Compress-Archive writes non-ASCII entry names without the
UTF-8 flag, which Windows Explorer decodes as ANSI (mojibake on GBK systems).
Python's zipfile always encodes names as UTF-8 and sets flag 0x800.

Usage: python zip_tree.py <src_dir> <out_zip>
"""
import os
import sys
import zipfile


def main() -> int:
    src, dst = sys.argv[1], sys.argv[2]
    base = os.path.dirname(os.path.abspath(src))
    count = 0
    with zipfile.ZipFile(dst, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
        for root, _dirs, files in os.walk(src):
            for f in files:
                p = os.path.join(root, f)
                z.write(p, os.path.relpath(p, base))
                count += 1
    print(f"zip_tree: {count} files -> {dst}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
