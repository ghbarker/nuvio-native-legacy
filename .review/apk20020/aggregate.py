#!/usr/bin/env python3
"""Verify fresh CI artifacts from this approved source; never build or publish."""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import os
import re
import struct
import sys
import xml.etree.ElementTree as ET
import zipfile

HERE = Path(__file__).resolve().parent
JOBS = ("native", "original-fontepref", "original-fontepref_file", "original-player_retido", "android", "jvm", "feature-contracts")
ARTIFACTS = dict(zip(JOBS, ("native", "original-fontepref", "original-fontepref_file", "original-player_retido", "android", "jvm47", "features")))
SUITES = {
    "fontepref": ("717d6226c8933a3fed0a744c861a72641493706c35c4522cd6c602f381a08780", "173a124acfc643af8e6312bcface4097fdd426743b8834ed726ebc6fa03a72e8"),
    "fontepref_file": ("f973b5476af03cb3d962d36afb85ff7b52855dd213dc53dacb4568cdb682bd20", "171912b66103bb66d256274505f0f9487751f8b0b66426943d8fddb06793a5f1"),
    "player_retido": ("7931a7e9af2c6572ab1e093720325eb27038f8b68bba2e2079097379b95a2eb1", "ac0581a7dddbd8777515b83776d8b2159a4715059153d11ddf6308c64be7cf78"),
}


def need(value, message):
    if not value:
        raise ValueError(message)


def sha(path):
    h = hashlib.sha256()
    with Path(path).open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        need(key not in result, "Duplicate JSON key: " + key)
        result[key] = value
    return result


def read_json(path):
    return json.loads(Path(path).read_text(), object_pairs_hook=unique_object)


def digest(value):
    need(isinstance(value, str) and re.fullmatch(r"[0-9a-f]{64}", value), "Missing SHA256")
    return value


def job_results(value):
    result = json.loads(value, object_pairs_hook=unique_object) if isinstance(value, str) else value
    need(set(result) == set(JOBS), "All seven required job results must be supplied")
    for key in JOBS:
        status = result[key].get("result") if isinstance(result[key], dict) else result[key]
        need(status == "success", "Required job did not succeed: " + key)
    return result


def source_binding(value, layer, plan, review):
    expected = plan["sources"][layer]
    need(value["source_commit"] == expected["commit"] and value["tree"] == expected["tree"], "Wrong " + layer + " source")
    need(value["review_commit"] == review, "Receipt belongs to another CI review commit")
    if "manifest_sha256" in value:
        need(value["manifest_sha256"] == expected["manifest_sha256"], "Wrong source manifest")
    if "plan_sha256" in value:
        need(value["plan_sha256"] == plan["_sha256"], "Wrong approved plan")
    if "bundle_sha256" in value:
        need(value["bundle_sha256"] == plan["bundle"]["sha256"], "Wrong source bundle")


def zero(value):
    return type(value) is int and value == 0


class Aggregate:
    def __init__(self, root, plan, review, stage=HERE):
        self.root, self.plan, self.review, self.stage = Path(root).resolve(), plan, review, Path(stage).resolve()
        need(self.root.is_dir(), "Artifact directory missing")
        self.files = {}
        self.proofs = {}
        self.dirs = {}
        for job, name in ARTIFACTS.items():
            directory = self.root / ("apk20020-" + name + "-" + review)
            need(directory.is_dir() and not directory.is_symlink(), "Required artifact missing: " + directory.name)
            self.dirs[job] = directory
        for path in self.root.rglob("*-source.json"):
            value = self.read(path)
            layer = value["layer"]
            need(layer in ("product", "test"), "Unknown source proof layer")
            source_binding(value, layer, plan, review)
            need(value["verified"] is True and value["source_clean"] is True and value["original_tests_tree_unchanged"] is True, "Unverified source materialization")
            need(value["review_only"] is False and value["base"] == plan["upstream"]["commit"], "Non-executable source proof")
            need(value["manifest_sha256"] == plan["sources"][layer]["manifest_sha256"] and value["bundle_sha256"] == plan["bundle"]["sha256"], "Incomplete source proof binding")
            self.proofs.setdefault(layer, set()).add(self.files[str(path.relative_to(self.root))])
        need(set(self.proofs) == {"product", "test"}, "Both source proofs are required")
        need(all(len(value) == 1 for value in self.proofs.values()), "Inconsistent source materialization receipts")

    def file(self, path, expected=None):
        path = Path(path)
        resolved = path.resolve()
        need(resolved.is_relative_to(self.root) and path.is_file() and not path.is_symlink(), "Missing or unsafe artifact file: " + str(path))
        value = sha(resolved)
        if expected is not None:
            need(value == digest(expected), "Artifact hash mismatch: " + str(path))
        self.files[str(path.relative_to(self.root))] = value
        return value

    def child(self, directory, relative):
        relative = Path(relative)
        need(not relative.is_absolute() and ".." not in relative.parts, "Unsafe relative artifact path")
        return Path(directory) / relative

    def read(self, path, expected=None):
        self.file(path, expected)
        return read_json(path)

    def one(self, job, name):
        paths = list(self.dirs[job].rglob(name))
        need(len(paths) == 1, "Missing or duplicated " + job + " receipt: " + name)
        return paths[0], self.read(paths[0])

    def bound(self, value, layer, proof_key="source_proof_sha256"):
        source_binding(value, layer, self.plan, self.review)
        need(value[proof_key] in self.proofs[layer], "Receipt uses an unknown source proof")

    def records(self, records, directory, name_key="label"):
        names = []
        need(records, "Empty execution records")
        for record in records:
            name = record[name_key]
            need(isinstance(name, str) and re.fullmatch(r"[a-zA-Z0-9_.-]+", name), "Unsafe record label")
            need(name not in names and zero(record["returncode"]) and not record.get("timed_out", False), "Failed, timed-out or duplicate execution: " + name)
            need(isinstance(record["command"], list) and record["command"], "Missing producer invocation")
            self.file(Path(directory) / (name + ".log"), record["log_sha256"])
            names.append(name)
        return set(names)

    def features(self):
        path, value = self.one("feature-contracts", "feature-contracts-verified.json")
        need(value["outcome"] == "passed" and value["groups"] == 20, "Focused feature stage incomplete")
        for layer in ("product", "test"):
            self.bound(value["source_bindings"][layer], layer, "proof_sha256")
        manifest = self.stage / "feature-contracts/manifest.json"
        need(value["fixture_manifest_sha256"] == sha(manifest) == self.plan["recipe_pins"]["feature-contracts/manifest.json"], "Wrong focused feature fixture graph")
        recipes = (("collection-state", None), ("subtitle-style", None), ("collection-settings", "all"), ("subtitle-outline", "tv"), ("subtitle-outline", "mobile"))
        expected = {name + "-" + layer + ("-" + ui if ui else "") + "-" + mode for layer in ("product", "test") for mode in ("normal", "asan") for name, ui in recipes}
        names = self.records(value["records"], path.parent)
        need(names == expected, "Missing or unexpected focused feature group")
        for record in value["records"]:
            need(record["passed"] is True and record["layer"] in ("product", "test"), "Failed feature producer")
            private = record["layer"] == "test"
            need(record["private"] is private and (("--private" in record["command"]) is private), "Wrong feature private macro selection")
            raw = self.read(self.child(path.parent, record["receipt_relative"]), record["receipt_sha256"])
            target = self.plan["sources"][record["layer"]]
            need(raw.get("passed") is True or raw.get("outcome") == "passed", "Nested feature assertions failed")
            need(raw["source_commit"] == target["commit"] and raw.get("source_tree", raw.get("tree")) == target["tree"], "Nested feature used different source")
            need(raw["private"] is private, "Nested feature private macro differs")
            sanitized = record["label"].endswith("-asan")
            if "sanitizers" in raw:
                need(raw["sanitizers"] in (("address,undefined", ["address", "undefined"]) if sanitized else (None, [], "")), "Nested feature sanitizer selection differs")
            else:
                need(raw.get("sanitized") is sanitized, "Nested feature sanitizer selection missing or wrong")
        need(value["libass_version"], "Actual system libass identity missing")
        for relative, expected_sha in value["raw_receipt_log_sha256"].items():
            self.file(self.child(path.parent, relative), expected_sha)
        return {"fresh_groups": 20, "caption_style_libass": True, "collection_settings_profiles_views_touch": True, "actual_libass_version": value["libass_version"]}

    def native(self):
        path, value = self.one("native", "native-verified.json")
        need(value["outcome"] == "passed" and value["sdk_sanity_passed"] is True, "Native SDK stage incomplete")
        for layer in ("product", "test"):
            self.bound(value["source_bindings"][layer], layer, "proof_sha256")
        legacy = self.stage.parent / "logs8/native"
        need(value["fixture_provenance_sha256"] == sha(legacy / "fixture-provenance.json"), "Wrong native fixture graph")
        need(value["portable_cache_synopsis_manifest_sha256"] == sha(self.stage / "preflight/cache-synopsis-portable-manifest.json"), "Wrong cached-status/synopsis fixture graph")
        need(value["contract_cc_sha256"] == hashlib.sha256(("#!/bin/sh\nexec " + value["compiler"] + ' -D_DARWIN_C_SOURCE=1 "$@"\n').encode()).hexdigest(), "Unexpected compatibility compiler wrapper")
        self.file(path.parent / "contract-cc", value["contract_cc_sha256"])
        names = self.records(value["records"], path.parent)
        expected = {"translations", "sdk-compile", "sdk-run", "actual-controls-product", "actual-controls-test", "actual513-dispatch", "actual513-owner"}
        for layer in ("product", "test"):
            for mode in ("normal", "asan"):
                for ui in ("tv", "mobile"):
                    expected.add("cache499-" + ui + "-" + layer + "-" + mode)
                for ui, modes in (("mobile", ("all",)), ("tv", ("close", "preview", "route"))):
                    for group in modes:
                        expected.add("synopsis477-" + ui + "-" + group + "-" + layer + "-" + mode)
            expected.update("link-" + layer + "-" + ui for ui in ("tv", "touch"))
        for group in ("original-parser", "exclusion-parser", "preparation", "profile-bank", "profile-scope", "worker"):
            expected.update("focused-" + group + "-" + mode + "-" + step for mode in ("normal", "asan") for step in ("compile", "run"))
        need(expected <= names and all(name in expected or name.startswith(("private-tv-", "private-touch-")) for name in names), "Required native execution closure is missing or unexpected")
        apps = value["full_apps"]
        need(len(apps) == 4 and {(x["layer"], x["ui"]) for x in apps} == {(layer, ui) for layer in ("product", "test") for ui in ("tv", "touch")}, "Four complete application links are required")
        modules = None
        for app in apps:
            binary = path.parent / ("app-" + app["layer"] + "-" + app["ui"])
            self.file(binary, app["sha256"])
            need(binary.stat().st_size == app["bytes"] and app["bytes"] > 0 and app["objects"] > 0, "Empty full application")
            if app["layer"] == "product":
                hashes = app["source_hashes"]
                need(len(hashes) == app["objects"], "Full-source object count differs")
                if modules is None:
                    modules = set(hashes)
                need(set(hashes) == modules, "TV/touch source sets differ")
                manifest = read_json(self.stage / self.plan["sources"]["product"]["manifest"])
                for entry in manifest["files"]:
                    if entry["path"].startswith("src/") and entry["path"].endswith(".c"):
                        need(hashes.get(entry["path"]) == entry["sha256"], "Changed C unit absent from complete app")
            else:
                need(app["translation_unit_sets_identical"] is True and app["private_macro_delta"] == ["-DNV_TEST_LOCAL_LOGS=1"], "Unexpected private app compilation scope")
                need({"src/ajustes.c", "src/app.c", "src/posplay.c"} <= set(app["new_private_objects"]), "Private macro-sensitive objects were reused")
                reused = app["same_invocation_dependency_bound_object_reuse"]
                need(len(reused) + len(app["new_private_objects"]) == app["objects"], "Unaccounted private app objects")
                for entry in reused:
                    evidence = entry["reuse_evidence"]
                    need(evidence["eligible"] is True and not evidence["changed_dependencies"] and not evidence["private_macro_sensitive_dependencies"] and evidence["checked_dependencies"], "Unproven object reuse")
                    need(evidence["compiler_macro_delta"] == ["-DNV_TEST_LOCAL_LOGS=1"], "Unexpected object macro delta")
        for layer in ("product", "test"):
            directory = path.parent / ("controls-" + layer)
            control = self.read(directory / "preflight-result.json")
            # The nested producer has no review-commit field. Its hash-checked
            # current native invocation and composition manifest bind it here.
            need(control["source_commit"] == self.plan["sources"][layer]["commit"] and control["tree"] == self.plan["sources"][layer]["tree"], "Wrong controls source")
            need(control["outcome"] == "passed" and control["layer"] == layer and control["complete_source_clean_after"] is True, "Actual controls incomplete")
            need(control["composition_manifest_sha256"] == self.plan["composition_manifest"]["sha256"], "Wrong controls composition")
            controls = self.records(control["records"], directory, "name")
            need(controls == {"player-" + ui + "-" + mode for ui in ("tv", "touch", "android") for mode in ("normal", "asan")} | {"app-" + ui + "-" + mode for ui in ("tv", "android") for mode in ("normal", "asan")} | {"posplay", "selectors"}, "Incomplete actual controls combinations")
            for mode in ("normal", "asan"):
                for ui in ("tv", "mobile"):
                    label = "cache499-" + ui + "-" + layer + "-" + mode
                    self.portable(path.parent / label, layer, ui, mode, {"metadata", "native", "caption", "refresh"}, "group")
                for ui, groups in (("mobile", ("all",)), ("tv", ("close", "preview", "route"))):
                    for group in groups:
                        label = "synopsis477-" + ui + "-" + group + "-" + layer + "-" + mode
                        self.portable(path.parent / label, layer, ui, mode, {"close", "preview", "route", "settings"} if group == "all" else {group}, "mode")
        return {"complete_apps": 4, "actual_controls_layers": 2, "cached_status_groups": 32, "synopsis_groups": 28, "records": len(names)}, modules

    def portable(self, directory, layer, ui, mode, groups, key):
        value = self.read(directory / "receipt.json")
        expected = self.plan["sources"][layer]
        need(value["passed"] is True and value["source_commit"] == expected["commit"] and value["source_tree"] == expected["tree"], "Wrong portable source or failed group")
        need(value["platform"] == ui and value["private"] is (layer == "test") and value["sanitizers"] == ("address,undefined" if mode == "asan" else None), "Wrong portable variant")
        rows = value["groups"]
        need(len(rows) == len(groups) and {r[key] for r in rows} == groups and all(zero(r["returncode"]) for r in rows), "Portable groups incomplete or failed")
        if key == "group":
            need(all(r["passed"] is True and zero(r["compile_returncode"]) and r["source_inputs_unchanged"] is True for r in rows), "Cached-status compilation failed")

    def originals(self, modules):
        count = 0
        for suite, hashes in SUITES.items():
            for mode in ("normal", "asan"):
                path, value = self.one("original-" + suite, "original-" + mode + "-verified.json")
                self.bound(dict(value["source_binding"], source_proof_sha256=value["source_proof_sha256"]), "product")
                need(value["outcome"] == "passed" and value["suite"] == suite and value["mode"] == mode, "Original suite/mode incomplete")
                need(value["script_unchanged"] is True and value["original_flags_unchanged"] is True, "Original tests were changed")
                need(value["original_test_sha256"] == {"tests/" + suite + ".c": hashes[0], "tests/" + suite + ".sh": hashes[1]}, "Wrong original assertions")
                need(value["sanitizers"] == (["address", "undefined"] if mode == "asan" else []), "Wrong original sanitizer mode")
                need(value["rpath_wrapper_sha256"] == sha(self.stage / "original3/cc_rpath.py"), "Unexpected original test wrapper")
                self.file(path.parent / (suite + ".log"), value["log_sha256"])
                omitted = {"src/main.c", "src/fontepref.c"} if suite != "player_retido" else {"src/main.c", "src/player.c", "src/video.c"}
                need(omitted <= modules and value["full_source_files_compiled"] == len(modules - omitted), "Original full-source closure incomplete")
                entries = value["compile_proofs"]
                need(entries and len({entry["file"] for entry in entries}) == len(entries), "Missing/duplicate original compile proofs")
                records = [self.read(self.child(path.parent, e["file"]), e["sha256"]) for e in entries]
                for record in records:
                    need(zero(record["returncode"]), "Original compiler failed")
                    added = [record["added_link_rpath"]] if record["added_link_rpath"] else []
                    need(record["effective_arguments"] == record["arguments"] + added, "Original flags changed")
                    need(("-fsanitize=address,undefined" in record["arguments"]) == (mode == "asan"), "Original compile sanitizer differs")
                matching = [record for record in records if "tests/" + suite + ".c" in record["arguments"]]
                need(len(matching) == 1 and set(matching[0]["arguments"]) & modules == modules - omitted, "Actual original translation units incomplete")
                need(matching[0]["added_link_rpath"] == value["only_added_flag"] and value["only_added_flag"].startswith("-Wl,-rpath,"), "Unexpected original link repair")
                if suite == "player_retido":
                    for module, macro in (("src/player.c", "-Dsessao_usuario=nv_test_usuario"), ("src/video.c", "-Dvideo_tocar=nv_base_video_tocar")):
                        partial = [r for r in records if module in r["arguments"]]
                        need(len(partial) == 1 and "-c" in partial[0]["arguments"] and macro in partial[0]["arguments"], "Original retained-player partial missing")
                count += 1
        return {"full_source_runs": count, "normal": 3, "asan_ubsan": 3}

    def android(self):
        paths = list(self.dirs["android"].rglob("build-receipt.json"))
        need(len(paths) == 2, "Exactly two fresh Android build receipts are required")
        apks = {}
        for path in paths:
            build = self.read(path)
            layer = build["layer"]
            need(layer in ("product", "test") and layer not in apks, "Duplicate Android layer")
            self.bound(build, layer)
            need(build["verified"] is True and build["source_restored_clean"] is True and build["fresh_build_of_both_arms"] is True, "Android build not complete/fresh/clean")
            need(build["prior_apk_proofs_reused"] is False and build["no_dispatch_release_upload_or_backend_actions"] is True, "Unexpected Android proof reuse/action")
            value = self.read(path.parent / "android-verified.json", build["verification_sha256"])
            self.bound(value, layer)
            need(value["outcome"] == "passed" and value["layer"] == layer and value["prior_apk_proofs_reused"] is False and value["release_or_backend_actions"] is False, "Android verification incomplete")
            need(value["apk_sha256"] == build["apk_sha256"] and value["apk_name"] == build["apk_name"], "APK receipt mismatch")
            apk = self.child(path.parent, value["apk_name"])
            self.file(apk, value["apk_sha256"])
            need(apk.stat().st_size == value["apk_bytes"] and value["apk_bytes"] > 0, "Wrong APK byte count")
            self.file(path.parent / "ci-transforms.json", build["ci_only_transforms_sha256"])
            need(value["ci_transforms_sha256"] == build["ci_only_transforms_sha256"], "Android mutation proof differs")
            ident = self.plan["identity"]
            need(value["certificate_sha256"] == ident["certificate_sha256"] and value["native_16kb_pages"] == "passed" and value["legacy_jni_overloads_preserved"] is True, "APK signer/16KB/JNI gate failed")
            need(value["package"] == (ident["package"] if layer == "test" else "space.nuvio.nativelegacy"), "Wrong installed package")
            if layer == "test":
                need(value["version_code"] == ident["version_code"] and value["version_name"] == ident["version_name"], "Wrong account-compatible update version")
            else:
                need(re.fullmatch(r"\d+\.\d+\.\d+", value["version_name"]), "Invalid product version")
                parts = list(map(int, value["version_name"].split(".")))
                need(value["version_code"] == parts[0] * 10000 + parts[1] * 100 + parts[2], "Wrong product version code")
            provider = value["file_provider"]
            need(provider == {"authority": value["package"] + ".atualizacao", "nonexported": True, "updater_path_only": True}, "FileProvider scope changed")
            arms = value["actual_arm_compiles"]
            need(len(arms) == 2 and {x["abi"] for x in arms} == {"arm64-v8a", "armeabi-v7a"}, "Both actual ARM compiles are required")
            need(value["private_log_opt_in"] is (layer == "test"), "Private diagnostics scope differs")
            for arm in arms:
                need(arm["builds"] and all(x["main_sources"] > 0 and x["report_endpoint_configured"] is True and x["private_native_opt_in"] is (layer == "test") for x in arm["builds"]), "Actual ARM compile settings incomplete")
                for entry in arm["builds"]:
                    digest(entry["compile_commands_sha256"])
            self.apk_dex(apk, value, layer)
            apks[layer] = {key: value[key] for key in ("apk_name", "apk_sha256", "apk_bytes", "package", "version_name", "version_code", "certificate_sha256")}
        need(set(apks) == {"product", "test"}, "Missing fresh Android layer")
        return apks

    def apk_dex(self, apk, value, layer):
        spec = importlib.util.spec_from_file_location("apk20020_dex_methods", self.stage / "android/dex_methods.py")
        parser = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(parser)
        player, dexes = parser.apk_methods(apk, "Lspace/nuvio/nativelegacy/NvPlayer;")
        need(dexes == value["apk_dex_sha256"], "Actual APK DEX hashes differ")
        lookups = value["all_player_jni_lookups"]
        need(len(lookups) >= 19 and len({tuple(x) for x in lookups}) == len(lookups), "Incomplete JNI lookups")
        need({"preparar", "prepararProximo", "prepararEstado", "cancelarPreparo", "abrirPreparada", "pararMantendoPreparo"} <= {x[0] for x in lookups}, "Preload JNI API missing")
        legacy = read_json(self.stage / "android/legacy-jni-descriptors.json")["methods"]
        for name, descriptor in lookups + [[x["name"], x["descriptor"]] for x in legacy]:
            need(player.get((name, descriptor), 0) & 8, "Actual APK JNI method absent/nonstatic")
        for klass, record in value["compiled_android_classes"].items():
            digest(record["class_sha256"]); digest(record["javap_sha256"])
            actual, _ = parser.apk_methods(apk, "L" + klass.replace(".", "/") + ";")
            need(record["methods"] and all(tuple(method) in actual and not actual[tuple(method)] & 8 for method in record["methods"]), "Actual APK download API missing")
        need(set(value["compiled_android_classes"]) == {"space.nuvio.nativelegacy." + x for x in ("NuvioActivity", "RegistroDownload", "RegistroDownload$Companion")}, "Download compiler/API evidence incomplete")
        private, _ = parser.apk_methods(apk, "Lspace/nuvio/nativelegacy/TestLocalLogs;")
        need(bool(private) == (layer == "test") and (layer != "test" or ("snapshot", "()[B") in private), "Actual APK private class scope differs")
        with zipfile.ZipFile(apk) as archive:
            names = archive.namelist()
            need(len(names) == len(set(names)), "Duplicate APK entries")
            for abi in ("arm64-v8a", "armeabi-v7a"):
                for lib in ("libmain.so", "libSDL2.so", "libcurl.so"):
                    need("lib/" + abi + "/" + lib in names, "Actual APK ARM library missing")
            for name in names:
                if name.startswith("lib/arm64-v8a/") and name.endswith(".so"):
                    elf = archive.read(name)
                    need(len(elf) >= 64 and elf[:7] == b"\x7fELF\x02\x01\x01", "Invalid actual ARM64 ELF")
                    header = struct.unpack_from("<16sHHIQQQIHHHHHH", elf)
                    need(header[1:3] == (3, 183) and header[9] >= 56 and header[10] > 0 and header[5] >= 64 and header[5] + header[9] * header[10] <= len(elf), "Wrong actual ARM64 ELF header")
                    loads = 0
                    for index in range(header[10]):
                        kind, _, offset, address, _, size, memory, align = struct.unpack_from("<IIQQQQQQ", elf, header[5] + index * header[9])
                        if kind == 1:
                            need(align >= 16384 and align & (align - 1) == 0 and offset % align == address % align and size <= memory and offset + size <= len(elf), "Actual APK LOAD is not 16KB compatible")
                            loads += 1
                        elif kind == 0x6474E552:
                            need((address + memory) % 16384 == 0, "Actual APK RELRO is not 16KB compatible")
                    need(loads > 0, "Actual ARM64 ELF has no LOAD segments")

    def jvm(self):
        path, value = self.one("jvm", "jvm47-verified.json")
        self.bound(value["source_binding"], "test")
        need(value["outcome"] == "passed" and value["fresh_contracts"] == 47 and value["helper_contracts"] == 32 and value["media3_contracts"] == 15 and value["historical_execution_reused"] is False, "All47 fresh JVM contracts are required")
        helper = self.read(path.parent / "helpers-verified.json", value["helper_receipt_sha256"])
        media = self.read(path.parent / "media3-verified.json", value["media3_receipt_sha256"])
        identity = self.read(path.parent / "java-identity.json")
        need(identity == value["java_identity"] and any('"21.' in line or '"21"' in line for line in identity["version_output"]), "Wrong JVM toolchain identity")
        for key in ("executable_sha256", "release_sha256", "modules_sha256"):
            digest(identity[key])
        for result, count, log in ((helper, 32, "helpers-run.log"), (media, 15, "media3-run.log")):
            self.bound(result["source_binding"], "test")
            need(result["outcome"] == "passed" and result["fresh_executed_contracts"] == count and result["source_binding"] == value["source_binding"] and result["java_identity"] == identity, "Fresh JVM component binding differs")
            self.file(path.parent / log, result["invocation"]["log_sha256"])
        legacy = self.stage.parent / "logs8"
        actual = self.read(path.parent / "kotlin/kotlin-verified.json", helper["actual_receipt_sha256"])
        self.bound(actual["source_binding"], "test")
        need(helper["runner_sha256"] == sha(legacy / "kotlin/run.py"), "Wrong JVM helper runner")
        need(actual["outcome"] == "passed" and actual["passed_contracts"] == 32 and [x["passed_contracts"] for x in actual["records"]] == [0, 26, 6], "Fresh helper executions incomplete")
        need([x["gate"] for x in actual["records"]] == ["compile", "public-opt-in-false", "private-opt-in-true"], "Wrong helper execution selection")
        for record in actual["records"]:
            self.file(path.parent / "kotlin" / (record["gate"] + ".log"), record["log_sha256"])
        need(actual["tool_hashes"] == {e["name"]: e["sha256"] for e in read_json(legacy / "kotlin/tools.json")}, "Wrong actual Kotlin compiler graph")
        prepared = self.read(path.parent / "media3-prepared.json", media["prepared_receipt_sha256"])
        need(prepared["source_binding"] == value["source_binding"] and prepared["java_identity"] == identity and prepared["assertions_modified"] is False and media["assertions_modified"] is False, "Media3 source/assertion binding differs")
        selection = read_json(legacy / "runtime/player-reuse-plan.json")["required_test_classes"]
        need(sum(map(len, selection.values())) == 15 and prepared["selected_classes"] == selection and media["selected_test_classes"] == selection and prepared["excluded_obsolete_log_methods"] == 5, "Wrong Media3 assertion selection")
        need(len(prepared["current_player_components"]) == 10 and prepared["fixture_manifest_sha256"] == sha(legacy / "runtime/original-fixture-manifest.json"), "Incomplete current Media3 closure")
        fixture = read_json(legacy / "runtime/original-fixture-manifest.json")
        need(prepared["fixture_files"] == [e for e in fixture["helper_files"] if not e["path"].endswith("/LocalLogsContractTest.kt")], "Media3 fixture graph differs")
        self.file(path.parent / "resolved-dependencies.json", prepared["resolved_dependencies_sha256"])
        dependencies = self.read(path.parent / "resolved-dependencies.json")["artifacts"]
        versions = {entry["component"] for entry in dependencies if entry["component"].startswith("androidx.media3:")}
        need(versions and all(entry.endswith(":1.8.0") for entry in versions), "Wrong actual Media3 dependencies")
        need(prepared["gradle_distribution_jars"] and prepared["probe_apks"] and prepared["generated_assets"], "Missing actual probe build evidence")
        logs = ["generate-media.log", "probe-build.log", "dependency-graph.log", "gradle-version.log", "ffmpeg-version.log"]
        need(len(prepared["invocations"]) == len(logs), "Incomplete probe producer invocations")
        for name, invocation in zip(logs, prepared["invocations"]):
            self.file(path.parent / name, invocation["log_sha256"])
        for record in prepared["tool_files"].values():
            digest(record["sha256"])
        need(set(prepared["tool_files"]) == {"adb", "emulator", "clang", "gradle"}, "Probe toolchain evidence incomplete")
        need(media["emulator"]["ro.build.version.sdk"] == "35" and media["emulator"]["ro.product.cpu.abi"] == "x86_64" and media["emulator"]["ro.build.fingerprint"], "Wrong fresh emulator")
        digest(media["emulator"]["system_image_source_properties_sha256"])
        seen = {}
        need(media["test_xml_sha256"], "Fresh instrumentation XML missing")
        for name, expected in media["test_xml_sha256"].items():
            xml = self.child(path.parent, name)
            self.file(xml, expected)
            for case in ET.fromstring(xml.read_text()).iter("testcase"):
                klass, name = case.attrib["classname"], case.attrib["name"].split("[")[0]
                need(klass in selection and name in selection[klass] and name not in seen.setdefault(klass, set()), "Unexpected or duplicate instrumentation assertion")
                need(not any(case.find(kind) is not None for kind in ("failure", "error", "skipped")), "Instrumentation assertion failed/skipped")
                seen[klass].add(name)
        need({key: sorted(names) for key, names in seen.items()} == {key: sorted(names) for key, names in selection.items()}, "Missing actual Media3 assertions")
        metrics = self.read(path.parent / "media3-runtime/metrics-collection.json")
        need(metrics["complete"] is True, "Startup observation collection incomplete")
        return {"fresh_helpers": 32, "fresh_media3": 15, "fresh_total": 47, "historical_execution_reused": False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--artifacts", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--job-results-json", default=os.environ.get("APK20020_JOB_RESULTS"))
    args = parser.parse_args()
    need(sys.flags.optimize == 0, "Python optimization would disable producer assertions")
    need(args.job_results_json, "Required CI job results missing")
    jobs = job_results(args.job_results_json)
    need(not args.output.exists(), "Refusing to replace an existing aggregate receipt")
    from require_plan import load_plan, verify_inherited, git
    plan, _ = load_plan(HERE / "plan.json")
    plan["_sha256"] = sha(HERE / "plan.json")
    review = os.environ.get("GITHUB_SHA", "")
    need(re.fullmatch(r"[0-9a-f]{40}", review), "Current GITHUB_SHA is required")
    need(git(HERE.parents[1], "rev-parse", "HEAD") == review, "Checkout differs from current CI review commit")
    verify_inherited(plan, HERE.parents[1])
    own = {"aggregate.py": sha(__file__), "plan.json": plan["_sha256"]}
    for entry in list(plan["sources"].values()) + [plan["composition_manifest"]]:
        name = entry.get("manifest", entry.get("path"))
        expected = entry.get("manifest_sha256", entry.get("sha256"))
        need(sha(HERE / name) == expected, "Approved source/composition manifest changed")
        own[name] = expected
    gate = Aggregate(args.artifacts, plan, review)
    features = gate.features()
    native, modules = gate.native()
    original = gate.originals(modules)
    android = gate.android()
    jvm = gate.jvm()
    receipt = {"format": 1, "outcome": "passed", "review_commit": review,
        "github_run_id": os.environ.get("GITHUB_RUN_ID"), "github_run_attempt": os.environ.get("GITHUB_RUN_ATTEMPT"),
        "plan_sha256": plan["_sha256"], "bundle_sha256": plan["bundle"]["sha256"],
        "source_bindings": plan["sources"], "job_results": jobs,
        "native": native, "features": features, "original_full_source": original, "android": android, "jvm": jvm,
        "recipe_inputs_sha256": own, "artifact_inputs_sha256": dict(sorted(gate.files.items())),
        "publish_release_refs_or_backend_actions": False,
        "scope": "This run's complete native links, original normal/sanitizer executions, fresh Android APK checks, and fresh47 isolated JVM/Media3 contracts. No new physical-device playback/timing claim."}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with args.output.open("x") as stream:
        stream.write(json.dumps(receipt, indent=2) + "\n")
    print("PASS all seven source-bound jobs; four complete native apps, six original runs, two fresh APKs, 47 fresh JVM contracts")


if __name__ == "__main__":
    try:
        main()
    except (AssertionError, KeyError, ValueError, OSError, zipfile.BadZipFile, struct.error, ET.ParseError) as error:
        raise SystemExit("Aggregate refused: " + str(error))
