"""Fail-closed bindings shared by the APK20022 execution stages."""
from pathlib import Path
import hashlib, json, os, subprocess

def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()

def load_plan(path, allow_review=False):
    path = Path(path).resolve()
    plan = json.loads(path.read_text())
    assert plan["format"] == 1 and plan["source_frozen"], "Incomplete source plan"
    regression = plan.get("required_pointer_regression", {})
    assert regression.get("bound") is True, "Logs12 real-render/pointer regression is not bound"
    assert regression.get("all_new_geometry_coverage_bound") is True, "Logs12 portrait reachability regression is not bound"
    assert regression.get("async_row_load_regression_bound") is True, "Logs12 asynchronous Showcase row-load regression is not bound"
    assert regression.get("fixture_manifest_sha256") and regression.get("invocations"), "Incomplete pointer regression contract"
    style = plan.get("required_settings_style_regression", {})
    assert style.get("bound") is True and style.get("fixture_manifest_sha256"), "Logs12 Settings Subtitle style entry regression is not bound"
    synopsis = plan.get("required_synopsis_percentage_regression", {})
    assert synopsis.get("bound") is True and synopsis.get("fixture_manifest_sha256"), "Logs12 synopsis percentage/migration regression is not bound"
    approved = plan["frozen"] and plan["root_reviewed"]
    assert approved or allow_review, "Root review is required before execution"
    if approved:
        assert sha(path) == os.environ.get("NUVIO_REVIEW_PLAN_SHA256"), "Wrong reviewed plan hash"
    return plan, approved

def git(source, *args):
    return subprocess.check_output(["git", "-C", str(source), *args], text=True).strip()

def verify_source(plan, proof, layer, source):
    expected = plan["sources"][layer]
    assert proof["verified"] and proof["layer"] == layer
    assert proof["source_clean"] and not proof["review_only"]
    assert proof["original_tests_tree_unchanged"]
    assert proof["source_commit"] == expected["commit"]
    assert proof["tree"] == expected["tree"]
    assert proof["manifest_sha256"] == expected["manifest_sha256"]
    assert proof["bundle_sha256"] == plan["bundle"]["sha256"]
    assert proof["base"] == plan["upstream"]["commit"]
    review = os.environ.get("GITHUB_SHA")
    assert review and proof["review_commit"] == review, "Proof belongs to another review run"
    assert git(source, "rev-parse", "HEAD") == expected["commit"]
    assert git(source, "rev-parse", "HEAD^{tree}") == expected["tree"]
    assert not git(source, "status", "--porcelain")
    assert git(source, "rev-parse", "HEAD:tests") == git(source, "rev-parse", proof["base"] + ":tests")

def verify_inherited(plan, review_root):
    for path, expected in plan["inherited_review"]["trees"].items():
        assert git(review_root, "rev-parse", "HEAD:" + path) == expected, path
        subprocess.run(["git", "-C", str(review_root), "diff", "--quiet", "HEAD", "--", path], check=True)
