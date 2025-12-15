#!/usr/bin/env python3
"""
convert_to_hiai.py

Small helper to convert GGUF/ggml models to a HiAI-compatible format (OM/ACL)
This is a wrapper that calls an external converter tool. The converter command
must be provided via the `HIAI_CONVERTER` environment variable as a template
containing `{in}` and `{out}` placeholders. Example:

  export HIAI_CONVERTER="/path/to/converter --input {in} --output {out} --format om"

If no converter is provided the script will print instructions and exit with
non-zero status so the calling pipeline can show the error.

This script does NOT implement a converter itself because HiAI tooling is
vendor-specific and not present in this environment.
"""
import argparse
import os
import glob
import subprocess
import sys


def find_gguf(model_dir: str):
    # search for gguf or ggml files in the given dir
    patterns = ["*.gguf", "*.GGUF", "*.ggml", "*.GGML"]
    for p in patterns:
        matches = glob.glob(os.path.join(model_dir, p))
        if matches:
            # return the first match
            return matches[0]
    return None


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("model_dir", help="Directory that contains GGUF/GGML model")
    parser.add_argument("--out-format", choices=["om", "acl"], default="om", help="Target HiAI format")
    parser.add_argument("--out-name", type=str, default=None, help="Optional output filename (without path)")
    args = parser.parse_args()

    model_dir = os.path.abspath(args.model_dir)
    if not os.path.isdir(model_dir):
        print(f"Model directory not found: {model_dir}")
        sys.exit(2)

    gg = find_gguf(model_dir)
    if gg is None:
        print(f"No GGUF/GGML model found in {model_dir}")
        sys.exit(3)

    out_name = args.out_name or (os.path.splitext(os.path.basename(gg))[0] + f".{args.out_format}")
    out_path = os.path.join(model_dir, out_name)

    converter_template = os.environ.get("HIAI_CONVERTER")
    if not converter_template:
        print("No HIAI converter configured. Please install vendor HiAI conversion tool and set the environment variable HIAI_CONVERTER.")
        print("Example (bash): export HIAI_CONVERTER='path/to/convert_tool --input {in} --output {out} --format om'")
        print("Aborting conversion.")
        sys.exit(4)

    # substitute placeholders {in} and {out} in the template
    cmd = converter_template.replace("{in}", gg).replace("{out}", out_path)
    # support passing complex templates by splitting via shell
    print(f"Running converter: {cmd}")
    try:
        subprocess.check_call(cmd, shell=True)
    except subprocess.CalledProcessError as e:
        print(f"Converter failed with exit {e.returncode}")
        sys.exit(e.returncode)

    if os.path.exists(out_path):
        print(f"Converted model written to: {out_path}")
        sys.exit(0)
    else:
        print("Converter reported success but output file not found.")
        sys.exit(5)


if __name__ == "__main__":
    main()
