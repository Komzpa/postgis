import importlib.util
import pathlib
import unittest


MODULE_PATH = pathlib.Path(__file__).resolve().parents[1] / "ci" / "ci_need_suite.py"
SPEC = importlib.util.spec_from_file_location("ci_need_suite", MODULE_PATH)
CI_NEED_SUITE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CI_NEED_SUITE)


class NeedForSuiteTest(unittest.TestCase):
    def test_dist_runs_for_release_relevant_inputs(self):
        for path in (
            "liblwgeom/lwgeom.c",
            "postgis/geography.sql.in",
            "doc/reference_constructor.xml",
            "regress/core/affine.sql",
            "utils/check_dist_manifest.sh",
        ):
            with self.subTest(path=path):
                need, reason = CI_NEED_SUITE.need_for_suite(
                    [path],
                    "dist",
                    "",
                    "none",
                )

                self.assertTrue(need)
                self.assertIn(path, reason)

    def test_dist_skips_unrelated_pull_request_inputs(self):
        need, reason = CI_NEED_SUITE.need_for_suite(
            ["NEWS"],
            "dist",
            "",
            "none",
        )

        self.assertFalse(need)
        self.assertEqual("no source distribution input changed", reason)


if __name__ == "__main__":
    unittest.main()
