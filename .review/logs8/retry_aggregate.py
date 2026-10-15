#!/usr/bin/env python3
"""Bind successful original Logs8 jobs plus fresh native and compatible-Android retries, without rewriting proof bytes."""
from pathlib import Path, PurePosixPath
import argparse
import hashlib
import json
import os
import shutil
import stat
import subprocess
import time
import zipfile

PLAN_SHA = "efac80b5b9436720e32e16a8124f9b49b8e71d7d70d244afe6d82640e804f48b"
ORIGINAL_AGGREGATE_SHA = "a58fa41f04f67414ae34fe12e675ea10888af00c8b4ea2f757894ed5b15336ac"
SCOPES = {"kotlin-player", "original-fontepref", "original-fontepref_file", "original-player_retido"}


def sha(path):
    h = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(chunk)
    return h.hexdigest()


def read(path):
    return json.loads(Path(path).read_text())


def write(path, value):
    Path(path).write_text(json.dumps(value, indent=2) + "\n")


def gh_json(repository, endpoint):
    return json.loads(subprocess.check_output(
        ["gh", "api", "repos/" + repository + "/" + endpoint], text=True, timeout=60))


def wait_job(repository, job_id, run_id, review_commit, deadline, expected_name):
    while True:
        job = gh_json(repository, "actions/jobs/" + str(job_id))
        assert job["id"] == job_id and job["run_id"] == run_id and job["head_sha"] == review_commit
        assert job["name"] == expected_name, (job["name"], expected_name)
        if job["status"] == "completed":
            assert job["conclusion"] == "success", (job_id, job["conclusion"])
            return job
        remaining = deadline - time.monotonic()
        assert remaining > 0, "Required original job did not finish within the bounded wait"
        print("Waiting for pinned job", job_id, expected_name, "status", job["status"], flush=True)
        time.sleep(min(15, remaining))


def validate_artifact(metadata, entry, run_id, review_commit):
    assert metadata["id"] == entry["id"] and metadata["name"] == entry["name"]
    assert not metadata["expired"] and metadata["digest"] == "sha256:" + entry["sha256"]
    assert metadata["workflow_run"]["id"] == run_id
    assert metadata["workflow_run"]["head_sha"] == review_commit


def extract_archive(archive, destination):
    destination = Path(destination)
    assert not destination.exists(), "Refusing to merge or overwrite previous proof files"
    with zipfile.ZipFile(archive) as z:
        infos = z.infolist()
        assert len(infos) == len({i.filename for i in infos}), "Duplicate ZIP entries"
        assert sum(i.file_size for i in infos) < 2 * 1024 ** 3, "Unbounded artifact expansion"
        for info in infos:
            relative = PurePosixPath(info.filename)
            assert not relative.is_absolute() and ".." not in relative.parts
            assert not stat.S_ISLNK(info.external_attr >> 16), "Symlink artifact member rejected"
        destination.mkdir(parents=True)
        z.extractall(destination)


def download_artifact(repository, entry, run_id, review_commit, out, scope, quarantine=False):
    metadata = gh_json(repository, "actions/artifacts/" + str(entry["id"]))
    validate_artifact(metadata, entry, run_id, review_commit)
    archive = out / "archives" / (scope + ".zip")
    with archive.open("xb") as stream:
        subprocess.run(["gh", "api", "repos/" + repository + "/actions/artifacts/" + str(entry["id"]) + "/zip"],
            stdout=stream, check=True, timeout=180)
    assert sha(archive) == entry["sha256"], "Actual downloaded ZIP digest mismatch"
    target = out / ("quarantine" if quarantine else "raw") / scope
    extract_archive(archive, target)
    return target, metadata


def same_expanded_tree(actual, downloaded):
    def files(root):
        root = Path(root)
        assert not any(p.is_symlink() for p in root.rglob("*"))
        return {str(p.relative_to(root)): sha(p) for p in root.rglob("*") if p.is_file()}
    assert files(actual) == files(downloaded), "Previously downloaded native files do not match verified current ZIP"


def adapt_aggregate(original, target):
    """Change only expected review origins; preserve every original behavioral assertion."""
    assert sha(original) == ORIGINAL_AGGREGATE_SHA, "Original aggregate changed; a fresh review is required"
    text = Path(original).read_text()
    replacements = [
        ("def binding(record,layer):", "def binding(record,layer,expected_review):"),
        ("record['review_commit']==os.environ['GITHUB_SHA']", "record['review_commit']==expected_review"),
        ("binding(v,layer)", "binding(v,layer,os.environ['GITHUB_SHA'])"),
        ("binding(kotlin['source_binding'],'test')", "binding(kotlin['source_binding'],'test',os.environ['LOGS8_ORIGINAL_REVIEW_SHA'])"),
        ("reuse['review_commit']==os.environ['GITHUB_SHA']", "reuse['review_commit']==os.environ['LOGS8_ORIGINAL_REVIEW_SHA']"),
        ("binding(d,d['layer'])", "binding(d,d['layer'],os.environ['GITHUB_SHA'] if d['layer']=='test' else os.environ['LOGS8_ORIGINAL_REVIEW_SHA'])"),
        ("binding(d['source_binding'],'product')", "binding(d['source_binding'],'product',os.environ['LOGS8_ORIGINAL_REVIEW_SHA'])"),
        ("d['review_commit']==os.environ['GITHUB_SHA']", "d['review_commit']==(os.environ['GITHUB_SHA'] if d['layer']=='test' else os.environ['LOGS8_ORIGINAL_REVIEW_SHA'])"),
    ]
    for old, new in replacements:
        assert text.count(old) == 1, (old, text.count(old))
        text = text.replace(old, new, 1)
    # The inherited aggregator locates the frozen plan relative to its script.
    text = text.replace("here=Path(__file__).resolve().parent;", "here=Path(os.environ['LOGS8_FROZEN_STAGE']).resolve();", 1)
    compile(text, str(target), "exec")
    Path(target).write_text(text)
    return [{"old": old, "new": new} for old, new in replacements]


def validate_manifest(manifest, plan, repository):
    assert manifest["format"] == 1 and manifest["frozen"]
    assert manifest["repository"] == repository and manifest["plan_sha256"] == PLAN_SHA
    assert manifest["original_run_id"] == 38679339730
    assert manifest["original_review_commit"] == "9204d4d71e8456e85ebfee329d7ee74609912262"
    assert {e["scope"] for e in manifest["retained"]} == SCOPES and len(manifest["retained"]) == 4
    assert len({e["job_id"] for e in manifest["retained"]}) == 4
    assert len({e["artifact"]["id"] for e in manifest["retained"]}) == 4
    excluded = manifest["excluded_failed_native"]
    assert excluded["job_id"] == 116240061458 and excluded["artifact_id"] == 11946857868
    for e in manifest["retained"]:
        assert e["artifact"]["name"] == "logs8-" + e["scope"] + "-" + manifest["original_review_commit"]
        assert e["artifact"]["id"] != excluded["artifact_id"] and e["job_id"] != excluded["job_id"]
        assert len(e["artifact"]["sha256"]) == 64
    assert plan["frozen"] and plan["sources"]["product"]["commit"] == "68b520db152b7473be3500d04e01ca5bd4424719"
    assert plan["sources"]["test"]["commit"] == "260c83a604b2a35787714a2946566d1dfd049704"
    standard = manifest["retained_android_product_phase"]
    assert standard["root_approved_successful_phase_reuse"] is True
    assert standard["job_id"] == 116240061121 and standard["job_conclusion"] == "failure"
    assert standard["step_name"] == "Build and verify clean standard product APK with both ARM cores"
    assert standard["step_number"] == 12
    assert standard["artifact"]["id"] == 11947769079
    assert standard["artifact"]["sha256"] == "f4e469ccca47189b67bc26286ffd51877b200a1001ce50277e1536d676adc99c"
    assert standard["apk_sha256"] == "d20c026d1772f708475102ddef349fd5b7002bf5e52c4478b8eeb79da1439c3d"


def fresh_artifact(a, out, deadline, current_run, current_sha, job_name, prefix, scope):
    jobs = gh_json(a.repository, "actions/runs/" + str(current_run) + "/jobs?per_page=100")
    assert jobs["total_count"] <= 100
    matched = [j for j in jobs["jobs"] if j["name"] == job_name]
    assert len(matched) == 1, (job_name, [j["name"] for j in jobs["jobs"]])
    job = wait_job(a.repository, matched[0]["id"], current_run, current_sha, deadline, job_name)
    artifacts = gh_json(a.repository, "actions/runs/" + str(current_run) + "/artifacts?per_page=100")
    assert artifacts["total_count"] <= 100
    matched = [d for d in artifacts["artifacts"] if d["name"] == prefix + current_sha]
    assert len(matched) == 1
    entry = {"id": matched[0]["id"], "name": matched[0]["name"],
        "sha256": matched[0]["digest"].removeprefix("sha256:")}
    raw, metadata = download_artifact(a.repository, entry, current_run, current_sha, out, scope)
    write(out / (scope + "-job.json"), job)
    write(out / (scope + "-artifact.json"), metadata)
    origin = {"run_id": current_run, "review_commit": current_sha, "job_id": job["id"],
        "job_name": job["name"], "conclusion": job["conclusion"], "artifact": entry}
    return raw, origin


def retain_standard_phase(a, manifest, out):
    e = manifest["retained_android_product_phase"]
    job = gh_json(a.repository, "actions/jobs/" + str(e["job_id"]))
    assert job["head_sha"] == manifest["original_review_commit"] and job["run_id"] == manifest["original_run_id"]
    assert job["status"] == "completed" and job["conclusion"] == "failure" and job["name"] == e["job_name"]
    step = [s for s in job["steps"] if s["number"] == e["step_number"] and s["name"] == e["step_name"]]
    assert len(step) == 1 and step[0]["status"] == "completed" and step[0]["conclusion"] == "success"
    raw, metadata = download_artifact(a.repository, e["artifact"], manifest["original_run_id"],
        manifest["original_review_commit"], out, "original-android-unfiltered", quarantine=True)
    candidates = list(raw.rglob("android-verified.json"))
    selected = [(p, read(p)) for p in candidates if read(p).get("layer") == "product"]
    assert len(selected) == 1
    file, proof = selected[0]
    assert proof["outcome"] == "passed" and proof["apk_sha256"] == e["apk_sha256"]
    assert proof["source_commit"] == "68b520db152b7473be3500d04e01ca5bd4424719"
    assert proof["review_commit"] == manifest["original_review_commit"]
    receipt = read(file.parent / "build-receipt.json")
    assert receipt["verified"] and receipt["source_restored_clean"] and receipt["layer"] == "product"
    assert receipt["apk_sha256"] == proof["apk_sha256"] and receipt["verification_sha256"] == sha(file)
    assert sha(file.parent / proof["apk_name"]) == e["apk_sha256"]
    destination = out / "raw" / "android-product"
    shutil.copytree(file.parent, destination)
    assert len(list(destination.rglob("android-verified.json"))) == 1
    assert not any("test" == read(p).get("layer") for p in destination.rglob("build-receipt.json"))
    write(out / "original-android-job.json", job)
    write(out / "original-android-artifact.json", metadata)
    return {"run_id": manifest["original_run_id"], "review_commit": manifest["original_review_commit"],
        "job_id": job["id"], "job_name": job["name"], "job_conclusion": "failure",
        "step_number": e["step_number"], "step_name": e["step_name"], "step_conclusion": "success",
        "root_approved_successful_phase_reuse": True, "artifact": e["artifact"],
        "apk_sha256": e["apk_sha256"], "old_private_test_excluded": True,
        "scope": "Only individually successful standard-product build/verification/restore phase; failed private-test APK and overall Android job are not passes"}


def collect(a):
    stage = a.plan.resolve().parent
    assert sha(a.plan) == PLAN_SHA
    plan = read(a.plan)
    manifest = read(a.retained_manifest)
    validate_manifest(manifest, plan, a.repository)
    current_sha = os.environ["GITHUB_SHA"]
    current_run = int(os.environ["GITHUB_RUN_ID"])
    assert len(current_sha) == 40 and current_sha != manifest["original_review_commit"]
    assert current_run != manifest["original_run_id"]
    out = a.output_dir.resolve()
    assert not out.exists(), "Use a new aggregate output directory"
    out.mkdir(parents=True)
    (out / "archives").mkdir()
    (out / "raw").mkdir()
    (out / "quarantine").mkdir()
    deadline = time.monotonic() + a.wait_seconds
    old_run = gh_json(a.repository, "actions/runs/" + str(manifest["original_run_id"]))
    assert old_run["head_sha"] == manifest["original_review_commit"]
    original_entries = []
    for e in manifest["retained"]:
        job = wait_job(a.repository, e["job_id"], manifest["original_run_id"],
            manifest["original_review_commit"], deadline, e["job_name"])
        raw, meta = download_artifact(a.repository, e["artifact"], manifest["original_run_id"],
            manifest["original_review_commit"], out, e["scope"])
        assert not list(raw.rglob("native-verified.json")), "Old native proof must never enter retained evidence"
        original_entries.append({"scope": e["scope"], "job_id": job["id"], "job_name": job["name"],
            "run_id": job["run_id"], "review_commit": job["head_sha"], "conclusion": job["conclusion"],
            "artifact": e["artifact"]})
        write(out / (e["scope"] + "-job.json"), job)
        write(out / (e["scope"] + "-artifact.json"), meta)
    standard_origin = retain_standard_phase(a, manifest, out)
    raw, native_origin = fresh_artifact(a, out, deadline, current_run, current_sha,
        a.native_job_name, "logs8-native-", "native")
    if a.native_results:
        same_expanded_tree(a.native_results, raw)
    assert len(list(raw.rglob("native-verified.json"))) == 1, "Fresh native complete pass is mandatory"
    raw_android, test_origin = fresh_artifact(a, out, deadline, current_run, current_sha,
        a.android_job_name, "logs8-android-test-", "android-test")
    test_proofs = list(raw_android.rglob("android-verified.json"))
    assert len(test_proofs) == 1 and read(test_proofs[0])["layer"] == "test"
    origin = {"repository": a.repository, "original": {"run_id": manifest["original_run_id"],
        "review_commit": manifest["original_review_commit"], "overall_run_success_required": False,
        "overall_run_status_at_collection": old_run["status"],
        "overall_run_conclusion_at_collection": old_run["conclusion"], "retained_jobs": original_entries},
        "fresh_native": native_origin, "fresh_android_test": test_origin,
        "retained_android_product_phase": standard_origin,
        "excluded_failed_native": {"run_id": manifest["original_run_id"],
            **manifest["excluded_failed_native"], "never_reused": True}}
    write(out / "validation-origin.json", origin)
    adapted = out / "_aggregate-review-binding-adapter.py"
    replacements = adapt_aggregate(stage / "aggregate.py", adapted)
    env = dict(os.environ, LOGS8_ORIGINAL_REVIEW_SHA=manifest["original_review_commit"], LOGS8_FROZEN_STAGE=str(stage))
    result_path = out / "all-gates-verified.json"
    result = subprocess.run(["python3", str(adapted), "--results", str(out / "raw"), "--output", str(result_path)],
        env=env, text=True, capture_output=True, timeout=120)
    (out / "aggregate.log").write_text(result.stdout + result.stderr)
    assert result.returncode == 0, result.stdout + result.stderr
    summary = read(result_path)
    assert summary["outcome"] == "passed" and summary["review_commit"] == current_sha
    assert summary["plan_sha256"] == PLAN_SHA
    summary.update({"format": 2, "validation_origin": origin,
        "retained_manifest_sha256": sha(a.retained_manifest), "aggregate_adapter_sha256": sha(Path(__file__)),
        "original_aggregate_sha256": ORIGINAL_AGGREGATE_SHA, "review_binding_adapter_sha256": sha(adapted),
        "review_binding_overrides": replacements,
        "raw_proof_files_unchanged": True, "old_failed_native_partial_outputs_reused": False})
    write(result_path, summary)
    print("Verified fresh complete native/private-Android passes, four successful original jobs and the approved successful standard-product phase", flush=True)
    print(result_path, flush=True)


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--plan", type=Path, required=True)
    p.add_argument("--retained-manifest", type=Path, required=True)
    p.add_argument("--repository", required=True)
    p.add_argument("--output-dir", type=Path, required=True)
    p.add_argument("--native-results", type=Path)
    p.add_argument("--native-job-name", default="native")
    p.add_argument("--android-job-name", default="android-test")
    p.add_argument("--wait-seconds", type=int, default=900)
    a = p.parse_args()
    assert 0 <= a.wait_seconds <= 900
    collect(a)


if __name__ == "__main__":
    main()
