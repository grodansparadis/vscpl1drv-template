#!/usr/bin/env python3
"""Make a new VSCP Level I driver from this template.

  tools/new_driver.py NAME DESTINATION

NAME is the short name of the new driver, for example "mydevice". The
template is copied to DESTINATION with everything renamed:

  vscpl1drv-template   -> vscpl1drv-mydevice   (library, project, files)
  template-driver.*    -> mydevice-driver.*
  CTemplateDriver      -> CMydeviceDriver
  TEMPLATE_...         -> MYDEVICE_...

The copy gets an empty README_CHANGES.md and version 0 of the current
month. Then: git init, put the code for your hardware in the HARDWARE
functions of src/NAME-driver.cpp, and update the configuration string in
parseConfig, src/driverinfo.h, docs/configure.md and the man page.
"""

import argparse
import datetime
import os
import re
import shutil
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# Not copied: build output, version control and this tool itself
SKIP_DIRS = {".git", ".vscode", ".vs", "__pycache__", "staging"}
SKIP_FILES = {
    os.path.join("tools", "new_driver.py"),
    os.path.join("tests", "test_new_driver.py"),
}
TEXT_EXT = {".cpp", ".h", ".txt", ".cmake", ".md", ".def", ".py", ".yml", ".yaml", ".json", ".html", ".sh", ""}


def names(name):
    upper = re.sub(r"[^A-Za-z0-9]", "_", name).upper()
    camel = "".join(p[:1].upper() + p[1:] for p in re.split(r"[^A-Za-z0-9]", name) if p)
    return [
        ("vscpl1drv-template", "vscpl1drv-" + name),
        ("VSCPL1DRV-TEMPLATE", "VSCPL1DRV-" + name.upper()),
        ("template-driver", name + "-driver"),
        ("CTemplateDriver", "C" + camel + "Driver"),
        ("TEMPLATE_", upper + "_"),
    ]


def rename(text, pairs):
    for old, new in pairs:
        text = text.replace(old, new)
    return text


def skipped(rel):
    parts = rel.split(os.sep)
    if any(p in SKIP_DIRS or p.startswith("build") for p in parts[:-1]):
        return True
    return rel in SKIP_FILES


def main(argv=None):
    ap = argparse.ArgumentParser(description="Make a new VSCP Level I driver from the template")
    ap.add_argument("name", help="short name of the driver, e.g. mydevice")
    ap.add_argument("destination", help="folder for the new driver (must not exist)")
    args = ap.parse_args(argv)

    if not re.fullmatch(r"[a-z][a-z0-9-]*", args.name) or args.name == "template":
        ap.error("NAME must be lower case letters, digits and '-', starting with a letter")
    dest = os.path.abspath(args.destination)
    if os.path.exists(dest):
        ap.error(dest + " already exists")

    pairs = names(args.name)
    count = 0
    for dirpath, dirnames, filenames in os.walk(ROOT):
        rel_dir = os.path.relpath(dirpath, ROOT)
        dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS and not d.startswith("build")]
        for f in filenames:
            rel = os.path.normpath(os.path.join(rel_dir, f))
            if skipped(rel):
                continue
            src = os.path.join(dirpath, f)
            out = os.path.join(dest, rename(rel, pairs))
            os.makedirs(os.path.dirname(out), exist_ok=True)
            if os.path.splitext(f)[1] in TEXT_EXT:
                with open(src, encoding="utf-8") as fh:
                    text = fh.read()
                with open(out, "w", encoding="utf-8", newline="") as fh:
                    fh.write(rename(text, pairs))
            else:
                shutil.copyfile(src, out)
            shutil.copymode(src, out)
            count += 1

    # A new driver starts with an empty change log and version 0 of this month
    with open(os.path.join(dest, "README_CHANGES.md"), "w", encoding="utf-8") as fh:
        fh.write("# Changes in vscpl1drv-%s\n\n## Unreleased\n\n- Made from vscpl1drv-template\n" % args.name)
    now = datetime.datetime.now(datetime.timezone.utc)
    version = os.path.join(dest, "src", "version.h")
    with open(version, encoding="utf-8") as fh:
        text = fh.read()
    yy, mm = now.year % 100, now.month
    text = re.sub(r"(#define\s+DRIVER_VERSION_YEAR\s+)\d+", r"\g<1>%d" % yy, text)
    text = re.sub(r"(#define\s+DRIVER_VERSION_MONTH\s+)\d+", r"\g<1>%d" % mm, text)
    text = re.sub(r"(#define\s+DRIVER_VERSION_PATCH\s+)\d+", r"\g<1>0", text)
    text = re.sub(r'(#define\s+DRIVER_VERSION_STR\s+)"[^"]*"', r'\g<1>"%d.%d.0"' % (yy, mm), text)
    text = re.sub(r"(#define\s+DRIVER_VERSION\s+)0x[0-9A-Fa-f]+UL", r"\g<1>0x%02X%02X0000UL" % (yy, mm), text)
    with open(version, "w", encoding="utf-8") as fh:
        fh.write(text)

    print("Made vscpl1drv-%s in %s (%d files)" % (args.name, dest, count))
    print("Next: cd %s && git init, then edit the HARDWARE functions in src/%s-driver.cpp" % (dest, args.name))
    return 0


if __name__ == "__main__":
    sys.exit(main())
