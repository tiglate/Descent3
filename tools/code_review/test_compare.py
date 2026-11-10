"""Regression tests for location-aware analyzer comparison."""

import unittest

from compare import unchanged_lines


class LocationMatchingTests(unittest.TestCase):
    def test_inserted_lines_do_not_make_an_existing_warning_new(self):
        before = "void example() {\n  bad_call();\n}\n"
        after = "// New comment\n" + before
        self.assertEqual(unchanged_lines(before, after)[2], 3)

    def test_ambiguous_replacement_is_not_accepted_as_the_old_warning(self):
        before = ("int old() {\n int *p = nullptr;\n return *p;\n}\n"
                  "int newer() {\n int *p = nullptr;\n return 1;\n}\n")
        after = ("int old() {\n int *p = nullptr;\n return 1;\n}\n"
                 "int newer() {\n int *p = nullptr;\n return *p;\n}\n")
        self.assertNotEqual(unchanged_lines(before, after).get(3), 7)


if __name__ == "__main__":
    unittest.main()
