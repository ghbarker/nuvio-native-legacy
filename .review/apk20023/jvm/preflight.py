#!/usr/bin/env python3
"""Offline structure/input checks only; executes no JVM, Gradle or source tests."""
from pathlib import Path
import argparse,ast,hashlib,importlib.util,json,re,sys
sys.dont_write_bytecode=True
p=argparse.ArgumentParser();p.add_argument('--legacy-root',type=Path,default=Path('/workspace/review/integration-logs8-20261015/build-stage'));p.add_argument('--source',type=Path,default=Path('/workspace/review/integration-apk20023-20261015/test'));a=p.parse_args()
here=Path(__file__).resolve().parent;legacy=a.legacy_root.resolve();source=a.source.resolve()
def sha(p):return hashlib.sha256(Path(p).read_bytes()).hexdigest()
ast.parse((here/'run.py').read_text());spec=importlib.util.spec_from_file_location('fresh47',here/'run.py');module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
manifest,files=module.fixture_graph(legacy);selection=json.loads((legacy/'runtime/player-reuse-plan.json').read_text())['required_test_classes']
assert sum(map(len,selection.values()))==15 and len(selection)==2
for klass,methods in selection.items():
 path=legacy/'runtime/fixture-graph'/('startup-diagnostics/StartupContractTest.kt' if klass.endswith('StartupContractTest') else 'probe/src/androidTest/java/space/nuvio/preloadtest/PreloadContractTest.kt')
 actual=re.findall(r'@Test\s+fun\s+(\w+)\s*\(',path.read_text());assert set(actual)==set(methods),(klass,actual)
components=[e for e in manifest['staged_player_helpers'] if not e['path'].endswith('/TestLocalLogs.kt')];assert len(components)==10
current={}
for entry in components:
 path=source/'android/app/src/main/java'/entry['path'];assert path.is_file();assert not re.search(r'\bTestLocalLogs\b',path.read_text());current[str(path.relative_to(source))]=sha(path)
job=json.loads((here/'job-fragment.json').read_text());steps=job['steps']
assert job['runs-on']=='ubuntu-24.04' and job['permissions']=={'contents':'read'}
assert any(s.get('uses')=='actions/setup-java@v4' and s['with']['java-version']=='21' for s in steps)
assert any(s.get('uses')=='gradle/actions/setup-gradle@v4' and s['with']['gradle-version']=='8.9' for s in steps)
assert any(s.get('uses')=='android-actions/setup-android@v3' and s['with']=={'packages':'platform-tools','log-accepted-android-sdk-licenses':False} for s in steps)
emulator=[s for s in steps if s.get('uses')=='reactivecircus/android-emulator-runner@v2'];assert len(emulator)==1 and emulator[0]['with']['api-level']==35 and emulator[0]['with']['arch']=='x86_64'
text=(here/'run.py').read_text();assert all(token in text for token in ['load_plan(HERE.parent','verify_source(plan,proof','verify_inherited(plan,HERE.parents[2])','executable_sha256','release_sha256','modules_sha256','historical_execution_reused'])
assert 'verify_reuse.py' not in text and '--offline-review' not in text
assert all(any('--phase '+phase in str(s) for s in steps) for phase in ['helpers','prepare-media3','media3','verify'])
assert not any('release create' in str(s) or 'workflow_dispatch' in str(s) for s in steps)
receipt={'outcome':'offline-preflight-only','scope':'Python/recipe/input/hash checks only; no Java/Kotlin/Gradle/media/emulator execution, no contracts counted as passed','fixture_files_verified':len(files),'unchanged_media3_assertions':15,'current_player_components':current,'compiler_tools':json.loads((legacy/'kotlin/tools.json').read_text()),'legacy_kotlin_runner_sha256':sha(legacy/'kotlin/run.py'),'fixture_manifest_sha256':sha(legacy/'runtime/original-fixture-manifest.json'),'recipe_files':{p.name:sha(p) for p in sorted(here.iterdir()) if p.is_file() and p.name not in ['preflight-receipt.json','manifest.json']}}
(here/'preflight-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print(json.dumps({'outcome':receipt['outcome'],'fixture_files':len(files),'assertions_byte_unchanged':15,'receipt':str(here/'preflight-receipt.json'),'sha256':sha(here/'preflight-receipt.json')}))
