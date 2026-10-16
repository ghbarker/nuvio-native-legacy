#!/usr/bin/env python3
"""Temporarily pause one fork workflow; PR ref writes belong to the reviewer."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import time
import urllib.error
import urllib.parse
import urllib.request

HERE = Path(__file__).resolve().parent
REPO = "ghbarker/nuvio-native-legacy"
BASE = "c2e50a9325719ffae85005c0542338f6b44878c3"
WORKFLOW_ID = 386951661
WORKFLOW_PATH = ".github/workflows/build-tpk.yml"
REVIEW_PATH = ".github/workflows/c2e-publish-window.yml"
BRANCH = "review/c2e-publish-window-20261016"
ALLOWED_BRANCHES = {
    431: ("feat/android-tv-mobile", "00b9c92bfeb5f49d962e4c66b680bc5b4dd7e456", "8979ef16c046a4b976d08dbbede82d81f9888a28"),
    538: ("feat/prioritize-continue-watching-497-20261015", "5f0ad732e2c7c9badbe22fb78b764bd0ea5cd571", "8c99638f71e1d9c47dde41c1b8c2a7e5cc62f342"),
    540: ("fix/cast-metadata-fallback-528-20261015", "5ae92423e7a605b43514aacd1bca9082439f0f27", "a064470826522dcf2b99767635be556b7555d097"),
    541: ("feat/loading-backdrop-options-508", "38d5168dd7b483aacbdf3b2ee5cef23d77426a93", "ad190469365670c94d4cc3ab40a7aa2dd44c2b66"),
    543: ("fix/removed-addon-catalogs-504", "85562a66bfd1882552d3f6fc2263764d1bdf442a", "bd6e611198955425172d9a59825204b63e02a53e"),
    546: ("fix/recommendation-backdrop-quality-503", "bbb74484e0f50b82357423fb349e0382117034ec", "6e2912703c8bc00ca9b6459fb1fa7c29f3007cfb"),
    547: ("feat/clock-focus-495-20261016", "d570cb3c1101c60fd16dd23bb6073d3efc559fd6", "7c7e670c3f18bc571d1e9b592e3e5526586a99d0"),
}
# Exact logical producers, not a claim that the original overall run passed.
# key -> (job ID, run ID, review SHA, attempt, workflow path)
ALLOWED_PRODUCERS = {
    "431-native": (117110420976, 38941625500, "619f4f62de245f473b134b9fc84829d9371df8bc", 1, ".github/workflows/maintainer-review.yml"),
    "431-android": (117110421629, 38941625500, "619f4f62de245f473b134b9fc84829d9371df8bc", 1, ".github/workflows/maintainer-review.yml"),
    "540-native": (117110421552, 38941625500, "619f4f62de245f473b134b9fc84829d9371df8bc", 1, ".github/workflows/maintainer-review.yml"),
    "541-native": (117110421391, 38941625500, "619f4f62de245f473b134b9fc84829d9371df8bc", 1, ".github/workflows/maintainer-review.yml"),
    "546-native": (117110421456, 38941625500, "619f4f62de245f473b134b9fc84829d9371df8bc", 1, ".github/workflows/maintainer-review.yml"),
    "538-native": (117124969339, 38945945983, "f6b0ad6a412a6362d49d843ccbd3395457415ce1", 1, ".github/workflows/c2e-native-recovery3.yml"),
    "543-native": (117118515978, 38944034395, "6ada5eecd077d90cdf32ee40946eb60c7be80c98", 1, ".github/workflows/c2e-native-recovery2.yml"),
    "547-native": (117114819348, 38942928463, "a3b5292934f68994c5d5ca5de4e6724083d66dd7", 1, ".github/workflows/c2e-native-recovery.yml"),
}
ROOT = HERE.parents[1]
OUT = ROOT / "publication-window"
MARKER = OUT / "window-marker.json"
RECEIPT = OUT / "window-receipt.json"
PLAN_BYTES = (HERE / "plan.json").read_bytes()
PLAN_SHA = hashlib.sha256(PLAN_BYTES).hexdigest()
PLAN = json.loads(PLAN_BYTES)


def require(ok, message):
    if not ok:
        raise RuntimeError(message)


def save(path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_suffix(".tmp")
    temporary.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n")
    temporary.replace(path)


def api(path, method="GET"):
    # No arbitrary write endpoints, repositories, methods or request bodies.
    require(path == f"/repos/{REPO}" or path.startswith(f"/repos/{REPO}/"), "Unexpected API repository")
    if method != "GET":
        require(method == "PUT" and path in (
            f"/repos/{REPO}/actions/workflows/{WORKFLOW_ID}/disable",
            f"/repos/{REPO}/actions/workflows/{WORKFLOW_ID}/enable"),
            "Write endpoint is outside the one-workflow allowlist")
    request = urllib.request.Request("https://api.github.com" + path, method=method,
        headers={"Authorization": "Bearer " + os.environ["GH_TOKEN"],
                 "Accept": "application/vnd.github+json",
                 "X-GitHub-Api-Version": "2022-11-28",
                 "User-Agent": "reviewed-c2e-publication-window"})
    with urllib.request.urlopen(request, timeout=8) as response:
        body = response.read(2 * 1024 * 1024)
        return json.loads(body) if body else None


def workflow():
    item = api(f"/repos/{REPO}/actions/workflows/{WORKFLOW_ID}")
    require(item["id"] == WORKFLOW_ID and item["path"] == WORKFLOW_PATH,
            "Registered workflow identity changed")
    return item


def identity():
    return {"repository": REPO, "workflow_id": WORKFLOW_ID,
            "workflow_path": WORKFLOW_PATH, "plan_sha256": PLAN_SHA,
            "run_id": int(os.environ["GITHUB_RUN_ID"]),
            "run_attempt": int(os.environ["GITHUB_RUN_ATTEMPT"]),
            "review_sha": os.environ["GITHUB_SHA"], "ref": os.environ["GITHUB_REF"]}


def preflight():
    require(PLAN["root_reviewed"] is True and PLAN["execution_authorized"] is True,
            "Publication window is closed pending root review")
    require(PLAN["all_eight_proofs_verified"] is True, "Eight independent proofs are pending")
    require(PLAN_SHA == os.environ["WINDOW_PLAN_SHA256"], "Plan bytes changed")
    require(PLAN["repository"] == REPO and PLAN["base"] == BASE
            and PLAN["workflow_id"] == WORKFLOW_ID and PLAN["workflow_path"] == WORKFLOW_PATH,
            "Plan scope changed")
    require(len(PLAN["branches"]) == 7 and {p["pr"]: (p["branch"], p["old_head"], p["candidate"])
            for p in PLAN["branches"]} == ALLOWED_BRANCHES, "PR branch allowlist changed")
    require(os.environ["GITHUB_REPOSITORY"] == REPO
            and os.environ["GITHUB_REF"] == "refs/heads/" + BRANCH
            and os.environ["GITHUB_EVENT_NAME"] == "push", "Wrong workflow invocation")
    require(hashlib.sha256(Path(__file__).read_bytes()).hexdigest() == PLAN["helper_sha256"],
            "Helper bytes changed")
    workflow_bytes = (ROOT / REVIEW_PATH).read_text()
    normalized = re.sub(r'"WINDOW_PLAN_SHA256": "[^"]+"',
                        '"WINDOW_PLAN_SHA256": "PLAN_SHA256"', workflow_bytes)
    normalized = normalized.replace('"if": "${{ true }}"', '"if": "${{ false }}"')
    require(hashlib.sha256(normalized.encode()).hexdigest() == PLAN["workflow_normalized_sha256"],
            "Workflow bytes changed")
    git = lambda *args: subprocess.check_output(["git", "-C", str(ROOT), *args], text=True).strip()
    require(git("rev-parse", "HEAD") == os.environ["GITHUB_SHA"]
            and git("rev-parse", "HEAD^") == BASE, "Review commit is not a direct child of base")
    changed = set(git("diff-tree", "--no-commit-id", "--name-only", "-r", "HEAD").splitlines())
    require(changed == {REVIEW_PATH, WORKFLOW_PATH,
                       ".review/c2e-publish-window/window.py", ".review/c2e-publish-window/plan.json"},
            "Unexpected review commit paths")
    require(not (ROOT / WORKFLOW_PATH).exists(), "Review tree must omit the unconditional TPK definition")
    require(git("rev-parse", f"{BASE}:{WORKFLOW_PATH}") == PLAN["base_workflow_blob"],
            "Base workflow identity changed")
    repository = api(f"/repos/{REPO}")
    require(repository["full_name"] == REPO and repository["fork"] is True,
            "This is not the expected fork")
    run = api(f"/repos/{REPO}/actions/runs/{os.environ['GITHUB_RUN_ID']}")
    require(run["head_sha"] == os.environ["GITHUB_SHA"] and run["head_branch"] == BRANCH
            and run["event"] == "push" and run["run_attempt"] == int(os.environ["GITHUB_RUN_ATTEMPT"])
            and run["path"] == REVIEW_PATH and run["repository"]["full_name"] == REPO,
            "Actual run identity changed")


def check_ci():
    require(len(PLAN["ci_jobs"]) == 8 and len({job["id"] for job in PLAN["ci_jobs"]}) == 8,
            "Exactly eight distinct successful jobs are required")
    require({p["logical_lane"]: (p["id"], p["run_id"], p["review_sha"], p["run_attempt"], p["workflow_path"])
            for p in PLAN["ci_jobs"]} == ALLOWED_PRODUCERS, "Exact validation producer origins changed")
    runs = {}
    observed = []
    for pin in PLAN["ci_jobs"]:
        pr, lane = pin["logical_lane"].split("-")
        expected_name = (f"PR{pr} macOS public tests and current app links" if lane == "native"
                         else f"PR{pr} Android ARM32/ARM64")
        require(pin["name"] == expected_name and pin["source_head"] == ALLOWED_BRANCHES[int(pr)][2],
                "Logical lane source or name changed")
        require(pin["proof_verified"] is True and re.fullmatch(r"[0-9a-f]{64}", pin["proof_receipt_sha256"] or ""),
                "This exact producer's independent proof is pending")
        if pin["run_id"] not in runs:
            run = api(f"/repos/{REPO}/actions/runs/{pin['run_id']}")
            require(run["head_sha"] == pin["review_sha"] and run["run_attempt"] == pin["run_attempt"]
                    and run["status"] == "completed" and run["path"] == pin["workflow_path"]
                    and run["event"] == "push" and run["repository"]["full_name"] == REPO,
                    "Actual validation run identity changed or is incomplete")
            # The original run legitimately failed its replaced lanes. Keep its
            # actual overall conclusion; only the eight exact jobs must pass.
            runs[pin["run_id"]] = run
        job = api(f"/repos/{REPO}/actions/jobs/{pin['id']}")
        require(job["run_id"] == pin["run_id"] and job["run_attempt"] == pin["run_attempt"]
                and job["name"] == pin["name"] and job["head_sha"] == pin["review_sha"]
                and job["status"] == "completed" and job["conclusion"] == "success",
                "Required fresh validation job has not passed")
        observed.append(dict(pin, actual_run_conclusion=runs[pin["run_id"]]["conclusion"],
                             actual_job_conclusion=job["conclusion"]))
    return observed


def heads():
    result = {}
    for pin in PLAN["branches"]:
        branch = urllib.parse.quote(pin["branch"], safe="")
        ref = api(f"/repos/{REPO}/git/ref/heads/{branch}")
        require(ref["ref"] == "refs/heads/" + pin["branch"] and ref["object"]["type"] == "commit",
                "Unexpected PR ref identity")
        result[str(pin["pr"])] = ref["object"]["sha"]
    return result


def restore():
    if not MARKER.exists():
        print("No marker: this job did not attempt to disable the workflow; no action.")
        return
    marker = json.loads(MARKER.read_text())
    require(all(marker.get(k) == v for k, v in identity().items()), "Restore marker belongs to another window")
    if marker.get("initial_state") != "active" or marker.get("disable_attempted") is not True:
        print("This window did not attempt disable; no restoration action.")
        return
    # Once restoration began, do not let a second ordinary cancellation interrupt it.
    signal.signal(signal.SIGTERM, signal.SIG_IGN)
    signal.signal(signal.SIGINT, signal.SIG_IGN)
    for attempt in range(6):
        try:
            state = workflow()["state"]
            require(state in ("active", "disabled_manually"), "Unexpected workflow state during restore")
            if state != "active":
                api(f"/repos/{REPO}/actions/workflows/{WORKFLOW_ID}/enable", "PUT")
            require(workflow()["state"] == "active", "Workflow restoration was not confirmed")
            marker.update(restored=True, restored_at_unix=time.time())
            save(MARKER, marker)
            print("Pinned fork workflow is active; restoration verified.")
            return
        except Exception:
            if attempt == 5:
                raise
            time.sleep(5)


def window():
    preflight()
    ci_producers = check_ci()
    require(not MARKER.exists(), "Existing window marker; refuse to reuse a disable window")
    before = heads()
    require(all(before[str(pin["pr"])] == pin["old_head"] for pin in PLAN["branches"]),
            "A PR ref moved before the publication window")
    require(workflow()["state"] == "active", "Pinned fork workflow was not initially active")
    marker = dict(identity(), initial_state="active", disable_attempted=False,
                  disabled_verified=False, restored=False, heads_before=before)
    save(MARKER, marker)
    receipt = dict(identity(), refs_are_read_only=True, all_heads_updated=False, passed=False,
                   ci_producers=ci_producers)
    try:
        marker["disable_attempted"] = True
        save(MARKER, marker)  # Persist before request, including a lost HTTP response.
        api(f"/repos/{REPO}/actions/workflows/{WORKFLOW_ID}/disable", "PUT")
        require(workflow()["state"] == "disabled_manually", "Disable was not confirmed")
        marker.update(disabled_verified=True, disabled_at_unix=time.time())
        save(MARKER, marker)
        print("Pinned fork workflow is disabled_manually. Root may now perform the seven leased ref updates.", flush=True)
        deadline = time.monotonic() + 600
        while time.monotonic() < deadline:
            current = heads()
            require(all(current[str(p["pr"])] in (p["old_head"], p["candidate"]) for p in PLAN["branches"]),
                    "A PR ref moved outside its two reviewed states")
            receipt["heads_last_observed"] = current
            if all(current[str(p["pr"])] == p["candidate"] for p in PLAN["branches"]):
                receipt["all_heads_updated"] = True
                break
            time.sleep(min(5, max(0, deadline - time.monotonic())))
        require(receipt["all_heads_updated"], "Ten-minute ref observation window expired")
    except BaseException as error:
        receipt["failure"] = type(error).__name__ + ": " + str(error)
        raise
    finally:
        try:
            restore()
        except BaseException as error:
            receipt["restoration_failure"] = type(error).__name__ + ": " + str(error)
            raise
        finally:
            receipt["restore_marker"] = json.loads(MARKER.read_text())
            receipt["passed"] = (receipt["all_heads_updated"] and
                                 receipt["restore_marker"].get("restored") is True and
                                 "failure" not in receipt and "restoration_failure" not in receipt)
            save(RECEIPT, receipt)


def interrupted(number, _frame):
    raise RuntimeError(f"Publication window interrupted by signal {number}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("window", "restore"))
    args = parser.parse_args()
    signal.signal(signal.SIGTERM, interrupted)
    signal.signal(signal.SIGINT, interrupted)
    require(os.environ.get("GH_TOKEN"), "GitHub token is unavailable")
    if args.mode == "window":
        window()
    else:
        # Independent always() step still verifies source/plan/run before any action.
        preflight()
        restore()
