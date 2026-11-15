#!/usr/bin/env python3
"""Test tools/new_driver.py: the copy is renamed everywhere."""

import os
import pathlib
import re
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

import new_driver  # noqa: E402


class NewDriverTest(unittest.TestCase):
    def test_copy_is_renamed(self):
        with tempfile.TemporaryDirectory() as tmp:
            dest = pathlib.Path(tmp) / "vscpl1drv-my-dev"
            self.assertEqual(new_driver.main(["my-dev", str(dest)]), 0)

            src = dest / "src"
            self.assertTrue((src / "my-dev-driver.cpp").exists())
            self.assertTrue((src / "my-dev-driver.h").exists())
            self.assertTrue((src / "vscpl1drv-my-dev.def").exists())
            self.assertTrue((dest / "man" / "vscpl1drv-my-dev.1.md").exists())
            self.assertFalse((dest / "tools" / "new_driver.py").exists())

            cmake = (dest / "CMakeLists.txt").read_text()
            self.assertIn("project(vscpl1drv-my-dev", cmake)

            code = "".join((src / f).read_text() for f in ("my-dev-driver.h", "my-dev-driver.cpp", "canal-api.cpp"))
            for expected in ("CMyDevDriver", "MY_DEV_FLAG_FD", "MY_DEV_DRIVER_H", '#include "my-dev-driver.h"'):
                self.assertTrue(expected in code, expected)
            # No identifier of the template is left in the code
            for path in src.iterdir():
                text = path.read_text()
                left = re.search(r"CTemplateDriver|TEMPLATE_|template-driver|vscpl1drv-template", text)
                self.assertIsNone(left, "%s still has %s" % (path.name, left.group(0) if left else ""))

            self.assertIn('LIBRARY "vscpl1drv-my-dev"', (src / "vscpl1drv-my-dev.def").read_text())
            self.assertTrue(re.search(r"DRIVER_VERSION_PATCH\s+0\b", (src / "version.h").read_text()))
            self.assertIn("vscpl1drv-my-dev", (dest / "README_CHANGES.md").read_text())

    def test_bad_names(self):
        with tempfile.TemporaryDirectory() as tmp:
            for bad in ("My", "1dev", "template", "a_b"):
                with self.assertRaises(SystemExit):
                    new_driver.main([bad, os.path.join(tmp, bad)])


if __name__ == "__main__":
    unittest.main(verbosity=2)
