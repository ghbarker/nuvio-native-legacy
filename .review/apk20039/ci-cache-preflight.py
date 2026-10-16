"""Reject GitHub Actions cache configurations that exceed its ten-key limit."""
import argparse
import json
from pathlib import Path


def validate(workflow):
    checked = []
    for job_name, job in workflow["jobs"].items():
        for index, step in enumerate(job.get("steps", [])):
            action = step.get("uses", "").lower()
            if not action.startswith(("actions/cache@", "actions/cache/restore@")):
                continue
            options = step["with"]
            assert options.get("key"), (job_name, index, "Missing primary cache key")
            count = 1 + len([s for s in options.get("restore-keys", "").splitlines() if s.strip()])
            assert count <= 10, (job_name, index, f"{count} cache keys exceeds GitHub's limit of 10")
            checked.append({"job": job_name, "step": index, "keys": count})
    return checked


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("workflow", type=Path)
    args = parser.parse_args()
    print(json.dumps(validate(json.loads(args.workflow.read_text()))))
