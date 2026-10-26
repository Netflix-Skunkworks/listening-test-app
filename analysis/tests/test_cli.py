import os
import subprocess
import sys

import pytest

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, ROOT)
from utility import parse_xml_results, plot_path  # noqa: E402

RES = os.path.join(ROOT, "resources")


def run_cli(*args):
    return subprocess.run([sys.executable, os.path.join(ROOT, "analyze_cli.py"), *args],
                          capture_output=True, text=True, cwd=ROOT)


@pytest.mark.parametrize("name", ["Subject 1: bob", 'a<b>c"d|e?f*g/h'])
def test_plot_path_has_no_illegal_chars(name):
    base = os.path.basename(plot_path("out", name))
    assert not set('<>:"/|?*') & set(base)


def test_parse_is_deterministic_and_sorted():
    a, _ = parse_xml_results(os.path.join(RES, "mushra_example_results"))
    b, _ = parse_xml_results(os.path.join(RES, "mushra_example_results") + os.sep)
    assert list(a) == list(b)
    assert list(a)[0].startswith("Subject 1:")


@pytest.mark.parametrize("sub,expected", [("mushra_example_results", 5), ("bs1116_example_results", 5)])
@pytest.mark.parametrize("trailing", ["", "/"] + (["\\"] if os.name == "nt" else []))
def test_cli_output_dir_variants(tmp_path, sub, expected, trailing):
    out = str(tmp_path / "out") + trailing
    r = run_cli("-i", os.path.join(RES, sub), "-o", out, "-psub", "-psti")
    assert r.returncode == 0, r.stderr
    pngs = [f for f in os.listdir(tmp_path / "out") if f.endswith(".png")]
    assert len(pngs) >= 2
    assert any(f.endswith("_all_items.png") for f in pngs)


def test_cli_ab_not_supported_message(tmp_path):
    r = run_cli("-i", os.path.join(RES, "ab_example_results"), "-o", str(tmp_path))
    assert r.returncode == 0 and "AB forced choice" in r.stdout


def test_cli_bad_input_dir(tmp_path):
    assert run_cli("-i", str(tmp_path / "nope")).returncode != 0


def test_cli_log_file_and_version(tmp_path):
    log = tmp_path / "l.log"
    r = run_cli("-i", os.path.join(RES, "mushra_example_results"), "-o", str(tmp_path), "--log", str(log))
    assert r.returncode == 0 and log.exists()
    assert "analyze_cli" in run_cli("--version").stdout


@pytest.mark.parametrize("module,func,sub", [("mushra", "plot_mushra_results", "mushra_example_results"),
                                              ("bs1116", "plot_bs1116_results", "bs1116_example_results")])
def test_plots_cap_yaxis(tmp_path, module, func, sub):
    import importlib
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    mod = importlib.import_module(module)
    listeners, info = parse_xml_results(os.path.join(RES, sub))
    getattr(mod, func)(listeners, info, str(tmp_path))
    assert plt.gcf().axes[0].get_ylim() == mod.YLIM
