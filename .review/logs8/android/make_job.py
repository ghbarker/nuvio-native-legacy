#!/usr/bin/env python3
"""Write a named job fragment; never load or slice another workflow's step list."""
import argparse,json
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--prefix',default='.review/logs8');p.add_argument('--output',type=Path,default=Path(__file__).with_name('job-fragment.json'));a=p.parse_args();root=a.prefix.rstrip('/');assert root and not root.startswith('/') and '..' not in Path(root).parts
steps=[
 {'uses':'actions/checkout@v4','with':{'persist-credentials':False,'fetch-depth':0}},
 {'name':'Materialize and verify final product and isolated test layers','run':f'''python3 {root}/materialize.py --layer product --output "$GITHUB_WORKSPACE/source-product" --proof "$GITHUB_WORKSPACE/build/review/logs8/product-source.json" --base-repository "$GITHUB_WORKSPACE"
python3 {root}/materialize.py --layer test --output "$GITHUB_WORKSPACE/source-test" --proof "$GITHUB_WORKSPACE/build/review/logs8/test-source.json" --base-repository "$GITHUB_WORKSPACE"'''},
 {'uses':'actions/setup-java@v4','with':{'distribution':'temurin','java-version':'17'}},
 {'uses':'android-actions/setup-android@v3','with':{'packages':'platform-tools','log-accepted-android-sdk-licenses':False}},
 {'name':'Install real Android and native build SDKs','run':'''sudo apt-get update -qq
sudo apt-get install -y ninja-build cmake nasm ffmpeg python3-pil zlib1g-dev
sdkmanager 'platforms;android-35' 'build-tools;35.0.0' 'ndk;27.2.12479018' 'cmake;3.22.1' '''.rstrip()},
 {'name':'Prepare private configuration and persistent debug signing','env':{'NUVIO_TOUCH_CORE_PROPERTIES':'${{ secrets.NUVIO_TOUCH_CORE_PROPERTIES }}','NUVIO_TOUCH_DEBUG_KEYSTORE_B64':'${{ secrets.NUVIO_TOUCH_DEBUG_KEYSTORE_B64 }}'},'run':f'python3 {root}/android/prepare_env.py --output "$RUNNER_TEMP/nuvio-logs8-private"'},
 {'uses':'actions/cache/restore@v4','id':'native-cache','with':{'path':'~/.cache/nuvio-android','key':"logs8-native-v2-${{ runner.os }}-${{ hashFiles('source-product/tools/android/deps.sh', 'source-product/tools/android/ffmpeg-android.sh', 'source-product/android/app/src/main/cpp/CMakeLists.txt') }}",'restore-keys':'logs8-native-v2-${{ runner.os }}-\nandroid-touch-native-v1-${{ runner.os }}-'}},
 {'uses':'actions/cache/restore@v4','id':'gradle-cache','with':{'path':'~/.gradle/caches\n~/.gradle/wrapper','key':"logs8-gradle-v2-${{ runner.os }}-${{ hashFiles('source-product/android/**/*.gradle.kts', 'source-product/android/gradle/wrapper/gradle-wrapper.properties', 'source-test/android/**/*.gradle.kts') }}",'restore-keys':'logs8-gradle-v2-${{ runner.os }}-\nandroid-touch-gradle-${{ runner.os }}-'}},
 {'name':'Record safe cache restoration outcomes','env':{'LOGS8_NATIVE_CACHE_PRIMARY':'${{ steps.native-cache.outputs.cache-primary-key }}','LOGS8_NATIVE_CACHE_MATCHED':'${{ steps.native-cache.outputs.cache-matched-key }}','LOGS8_NATIVE_CACHE_HIT':'${{ steps.native-cache.outputs.cache-hit }}','LOGS8_GRADLE_CACHE_PRIMARY':'${{ steps.gradle-cache.outputs.cache-primary-key }}','LOGS8_GRADLE_CACHE_MATCHED':'${{ steps.gradle-cache.outputs.cache-matched-key }}','LOGS8_GRADLE_CACHE_HIT':'${{ steps.gradle-cache.outputs.cache-hit }}'},'run':f'python3 {root}/android/record_cache.py --output build/review/logs8/cache-restoration.json'},
 {'name':'Build and verify clean standard product APK with both ARM cores','run':f'python3 {root}/android/run.py --layer product --source source-product --source-proof build/review/logs8/product-source.json --output build/review/logs8/android-product --jobs 2'},
 {'name':'Build and verify account-compatible Logs8 update with isolated diagnostics','run':f'python3 {root}/android/run.py --layer test --source source-test --source-proof build/review/logs8/test-source.json --output build/review/logs8/android-test --jobs 2'},
 {'name':'Record only safe cache/output presence','id':'outputs','if':'always()','run':'''if [[ -d "$HOME/.cache/nuvio-android" ]]; then echo 'native_cache=true' >> "$GITHUB_OUTPUT"; fi
if [[ -d "$HOME/.gradle/caches" || -d "$HOME/.gradle/wrapper" ]]; then echo 'gradle_cache=true' >> "$GITHUB_OUTPUT"; fi'''},
 {'uses':'actions/cache/save@v4','if':"always() && steps.outputs.outputs.native_cache == 'true' && steps.native-cache.outputs.cache-hit != 'true' && steps.native-cache.outputs.cache-primary-key != ''",'with':{'path':'~/.cache/nuvio-android','key':'${{ steps.native-cache.outputs.cache-primary-key }}'}},
 {'uses':'actions/cache/save@v4','if':"always() && steps.outputs.outputs.gradle_cache == 'true' && steps.gradle-cache.outputs.cache-hit != 'true' && steps.gradle-cache.outputs.cache-primary-key != ''",'with':{'path':'~/.gradle/caches\n~/.gradle/wrapper','key':'${{ steps.gradle-cache.outputs.cache-primary-key }}'}},
 {'uses':'actions/upload-artifact@v4','if':'always()','with':{'name':'logs8-android-${{ github.sha }}','path':'build/review/logs8/','if-no-files-found':'error','retention-days':14}},
 {'name':'Delete private runner configuration','if':'always()','run':f'python3 {root}/android/cleanup_private.py --directory "$RUNNER_TEMP/nuvio-logs8-private"'}
]
job={'name':'Logs8 clean product and compatible test APK (ARM32/ARM64)','runs-on':'ubuntu-24.04','timeout-minutes':90,'permissions':{'contents':'read'},'env':{'LOGS8_REVIEW_ROOT':root,'NUVIO_ANDROID_JOBS':'2','NUVIO_BUILD_JOBS':'2'},'steps':steps};a.output.write_text(json.dumps(job,indent=2)+'\n');print('Named Android job fragment written:',a.output)
