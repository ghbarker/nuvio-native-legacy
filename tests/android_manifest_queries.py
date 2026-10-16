#!/usr/bin/env python3
"""Check installer restrictions on package-visibility queries (no Android SDK).

Android accepts broader filters on activities than it accepts in <queries>.
See AOSP android-15.0.0_r1 ParsingPackageUtils.parseQueries: each query intent
may contain at most one action, MIME type, scheme and host. AAPT compilation
alone does not enforce these install-time restrictions.
"""
import argparse
from pathlib import Path
import sys
import unittest
import xml.etree.ElementTree as ET

ANDROID = "{http://schemas.android.com/apk/res/android}"


def query_errors(root):
    """Return the Android installer cardinality errors, scoped to queries."""
    errors = []
    for index, intent in enumerate(root.findall("./queries/intent"), 1):
        actions = {item.get(ANDROID + "name") for item in intent.findall("action")}
        actions.discard(None)
        schemes = {item.get(ANDROID + "scheme") for item in intent.findall("data")}
        schemes.discard(None)
        types = {item.get(ANDROID + "mimeType") for item in intent.findall("data")}
        types.discard(None)
        # IntentFilter.getHosts() includes each declared authority (including
        # authorities with the same host but a different port).
        hosts = [item.get(ANDROID + "host") for item in intent.findall("data")
                 if item.get(ANDROID + "host") is not None]
        if not actions and not schemes and not types:
            errors.append((index, "intent requires an action or data"))
        for name, values in (("action", actions), ("data type", types),
                             ("data scheme", schemes), ("data host", hosts)):
            if len(values) > 1:
                errors.append((index, "intent may have at most one " + name))
    return errors


def manifest_contract(root):
    """Check external-player visibility and keep existing discovery queries."""
    errors = ["query %d: %s" % error for error in query_errors(root)]
    intents = root.findall("./queries/intent")
    for scheme in ("http", "https"):
        if not any(
            {a.get(ANDROID + "name") for a in intent.findall("action")} ==
            {"android.intent.action.VIEW"}
            and {d.get(ANDROID + "scheme") for d in intent.findall("data")
                 if d.get(ANDROID + "scheme") is not None} == {scheme}
            and {d.get(ANDROID + "mimeType") for d in intent.findall("data")
                 if d.get(ANDROID + "mimeType") is not None} == {"video/*"}
            for intent in intents
        ):
            errors.append("missing %s video VIEW query" % scheme)
    packages = {p.get(ANDROID + "name") for p in root.findall("./queries/package")}
    for package in ("com.mxtech.videoplayer.ad", "com.mxtech.videoplayer.pro"):
        if package not in packages:
            errors.append("missing required-header player visibility: " + package)
    for action, category in (
        ("android.speech.action.RECOGNIZE_SPEECH", None),
        ("android.speech.RecognitionService", None),
        ("android.intent.action.MAIN", "android.intent.category.LEANBACK_LAUNCHER"),
        ("android.intent.action.MAIN", "android.intent.category.LAUNCHER"),
    ):
        if not any(
            any(a.get(ANDROID + "name") == action for a in intent.findall("action"))
            and (category is None or any(c.get(ANDROID + "name") == category
                                         for c in intent.findall("category")))
            for intent in intents
        ):
            errors.append("missing existing query: " + (category or action))
    return errors


def document(contents, in_queries=True):
    wrapper = "queries" if in_queries else "application/activity"
    opening = "".join("<%s>" % tag for tag in wrapper.split("/"))
    closing = "".join("</%s>" % tag for tag in reversed(wrapper.split("/")))
    return ET.fromstring(
        '<manifest xmlns:android="http://schemas.android.com/apk/res/android">'
        + opening + contents + closing + "</manifest>")


class InstallerQueryRules(unittest.TestCase):
    def test_action_only(self):
        self.assertEqual([], query_errors(document(
            '<intent><action android:name="example.OPEN"/></intent>')))

    def test_data_only(self):
        self.assertEqual([], query_errors(document(
            '<intent><data android:scheme="https"/></intent>')))

    def test_one_value_per_dimension_across_data_elements(self):
        self.assertEqual([], query_errors(document(
            '<intent><action android:name="example.OPEN"/>'
            '<data android:scheme="https"/><data android:mimeType="video/*"/>'
            '<data android:host="media.example"/></intent>')))

    def test_duplicate_actions_and_schemes_are_deduplicated(self):
        self.assertEqual([], query_errors(document(
            '<intent><action android:name="example.OPEN"/>'
            '<action android:name="example.OPEN"/><data android:scheme="https"/>'
            '<data android:scheme="https"/></intent>')))

    def test_multiple_actions_rejected(self):
        self.assertIn((1, "intent may have at most one action"), query_errors(document(
            '<intent><action android:name="example.OPEN"/>'
            '<action android:name="example.EDIT"/></intent>')))

    def test_multiple_schemes_rejected(self):
        self.assertIn((1, "intent may have at most one data scheme"), query_errors(document(
            '<intent><data android:scheme="http"/><data android:scheme="https"/>'
            '</intent>')))

    def test_multiple_mime_types_rejected(self):
        self.assertIn((1, "intent may have at most one data type"), query_errors(document(
            '<intent><data android:mimeType="video/*"/>'
            '<data android:mimeType="audio/*"/></intent>')))

    def test_multiple_hosts_rejected(self):
        self.assertIn((1, "intent may have at most one data host"), query_errors(document(
            '<intent><data android:scheme="https" android:host="one.example"/>'
            '<data android:host="two.example"/></intent>')))

    def test_hosts_with_different_ports_are_separate_authorities(self):
        self.assertIn((1, "intent may have at most one data host"), query_errors(document(
            '<intent><data android:scheme="https" android:host="one.example" '
            'android:port="80"/><data android:host="one.example" android:port="81"/>'
            '</intent>')))

    def test_empty_query_rejected(self):
        self.assertEqual([(1, "intent requires an action or data")],
                         query_errors(document("<intent/>")))

    def test_category_only_query_rejected(self):
        self.assertEqual([(1, "intent requires an action or data")], query_errors(document(
            '<intent><category android:name="example.CATEGORY"/></intent>')))

    def test_every_query_is_checked(self):
        self.assertEqual([(2, "intent may have at most one data scheme")],
                         query_errors(document(
            '<intent><action android:name="example.OPEN"/></intent>'
            '<intent><data android:scheme="http"/><data android:scheme="https"/>'
            '</intent>')))

    def test_activity_intent_filters_allow_multiple_schemes(self):
        # The exact same data pair is legal outside package-visibility queries.
        self.assertEqual([], query_errors(document(
            '<intent-filter><action android:name="example.OPEN"/>'
            '<data android:scheme="http"/><data android:scheme="https"/>'
            '</intent-filter>', in_queries=False)))

    def test_split_video_queries_valid(self):
        self.assertEqual([], query_errors(document(
            '<intent><action android:name="android.intent.action.VIEW"/>'
            '<data android:scheme="http" android:mimeType="video/*"/></intent>'
            '<intent><action android:name="android.intent.action.VIEW"/>'
            '<data android:scheme="https" android:mimeType="video/*"/></intent>')))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path, default=
                        Path(__file__).resolve().parents[1] /
                        "android/app/src/main/AndroidManifest.xml")
    args = parser.parse_args()
    suite = unittest.defaultTestLoader.loadTestsFromTestCase(InstallerQueryRules)
    result = unittest.TextTestRunner(verbosity=1).run(suite)
    if not result.wasSuccessful():
        return 1
    try:
        root = ET.parse(args.manifest).getroot()
    except (OSError, ET.ParseError) as error:
        print("android_manifest_queries: " + str(error), file=sys.stderr)
        return 1
    errors = manifest_contract(root)
    if errors:
        for error in errors:
            print("android_manifest_queries: " + error, file=sys.stderr)
        return 1
    print("android_manifest_queries: %d parser cases passed; %d query intents valid" %
          (result.testsRun, len(root.findall("./queries/intent"))))
    return 0


if __name__ == "__main__":
    sys.exit(main())
