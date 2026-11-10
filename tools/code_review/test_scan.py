"""Tests for selecting the compilation configuration used by the baseline."""

import unittest

from scan import configuration, select_entries


class ConfigurationTests(unittest.TestCase):
    def test_multi_config_selection_is_independent_of_order(self):
        entries = [
            {"directory": "/project", "file": "a.cpp", "arguments": ["c++", '-DCMAKE_INTDIR="Release"']},
            {"directory": "/project", "file": "a.cpp", "command": r'c++ -DCMAKE_INTDIR=\"Debug\"'},
        ]
        self.assertEqual(configuration(entries[1]), "Debug")
        self.assertEqual(select_entries(entries, {"/project/a.cpp"}, "Debug", None)["/project/a.cpp"], entries[1])
        self.assertEqual(select_entries(list(reversed(entries)), {"/project/a.cpp"}, "Debug", None)["/project/a.cpp"], entries[1])

    def test_single_config_requires_matching_cache(self):
        entry = {"directory": "/project", "file": "a.cpp", "arguments": ["c++"]}
        self.assertEqual(select_entries([entry], {"/project/a.cpp"}, "Debug", "Debug"), {"/project/a.cpp": entry})
        self.assertFalse(select_entries([entry], {"/project/a.cpp"}, "Debug", "Release"))
        self.assertFalse(select_entries([entry], {"/project/a.cpp"}, "Debug", None))


if __name__ == "__main__":
    unittest.main()
