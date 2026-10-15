"""Private selected-art trace checks; no GL, image download or full-cache build claim."""
from pathlib import Path
import argparse, hashlib, json, os, platform, subprocess

p = argparse.ArgumentParser()
p.add_argument('--source', type=Path, required=True)
p.add_argument('--out', type=Path, required=True)
p.add_argument('--sanitize', type=int, choices=[0, 1], required=True)
p.add_argument('--helpers-only', action='store_true', help='Local bounded preflight; defer full-detail syntax checks to CI.')
a = p.parse_args()
r = Path(__file__).resolve().parent
s = a.source.resolve()
o = a.out.resolve()
o.mkdir(parents=True, exist_ok=False)
sha = lambda p: hashlib.sha256(Path(p).read_bytes()).hexdigest()
paths = ['src/detail.c', 'src/tex_cache.c', 'src/tex_cache.h']
source_hashes = {n: sha(s / n) for n in paths}

def without_trace(text):
    result = []
    depth = 0
    for line in text.splitlines():
        if line.strip() == '#ifdef NV_TEST_LOCAL_LOGS':
            assert not depth
            depth = 1
        elif depth:
            if line.lstrip().startswith(('#if ', '#ifdef ', '#ifndef ')):
                depth += 1
            elif line.strip() == '#endif':
                depth -= 1
        elif line.strip():
            result.append(line)
    assert not depth
    return '\n'.join(result) + '\n'

baseline = json.loads((r / 'baseline.json').read_text())
for n in paths:
    actual = without_trace((s / n).read_text())
    assert hashlib.sha256(actual.encode()).hexdigest() == baseline['macro_off_nonblank_sha256'][n], n
detail = (s / 'src/detail.c').read_text()
assert detail.count('arteTesteAbrir();') == 1
assert 'arteTesteRegistrar(arteAtual, arteDetalheEhPoster(idx), comTrailer);' in detail
assert 'arteTesteRegistrar(arte, artePoster, comTrailer);' in detail
trace_start = detail.index('#ifdef NV_TEST_LOCAL_LOGS', detail.index('static int  arteFixaPoster;'))
trace_end = detail.index('\n#endif', trace_start) + len('\n#endif')
trace = o / 'actual-detail-trace.inc'
trace.write_text('#include <SDL2/SDL.h>\n#include "catalogo.h"\n#include "ajustes.h"\n'
                 '#include "fundo.h"\n#include "tex_cache.h"\n#include <strings.h>\n'
                 '// Typed selected-index boundary; actual opening/render hooks checked above.\n'
                 'static int idx;\n' + detail[trace_start:trace_end] + '\n')

cache = (s / 'src/tex_cache.c').read_text()
def function(signature):
    start = cache.index(signature)
    return cache[start:cache.index('\n}', start) + 2]
start = cache.index('typedef struct {\n  char caminho[')
item = cache[start:cache.index('} Item;', start) + len('} Item;')]
enum_start = cache.index('typedef enum { VAZIO=0,')
enum = cache[enum_start:cache.index(';', enum_start) + 1]
prefix = '''#include <assert.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <SDL2/SDL.h>
#include "tex_cache.h"
'''
state = '''
static Item itens[8];
static int nMax = 8, qualidadeImg = 2;
struct SDL_mutex { pthread_mutex_t mutex; };
static struct SDL_mutex mutex = { PTHREAD_MUTEX_INITIALIZER };
static SDL_mutex *mtx = &mutex;
int SDL_LockMutex(SDL_mutex *m) { return pthread_mutex_lock(&m->mutex); }
int SDL_UnlockMutex(SDL_mutex *m) { return pthread_mutex_unlock(&m->mutex); }
// Existing device/quality cap policy is a typed boundary, not under test here.
static int tetoDoHeroi(void) { return 1920; }
'''
generated = o / 'actual-cache-snapshot.c'
generated.write_text(prefix + enum + '\n' + item + state + '\n' + '\n'.join([
    function('static unsigned long hashCaminho('),
    function('unsigned long tex_hash_public('),
    function('static int acharIndice(const char *caminho, unsigned long h) {'),
    function('int tex_teste_arte('),
]) + '\n' + (r / 'cache-boundary.c').read_text())

darwin = platform.system() == 'Darwin'
flags = ['-std=gnu11', '-O1', '-g', '-ffunction-sections', '-fdata-sections',
         '-DNV_TEST_LOCAL_LOGS=1', '-I' + str(r / 'sdk'), '-I' + str(s / 'src'),
         '-DDETAIL_TRACE="' + str(trace) + '"']
if not darwin:
    flags += ['-fno-pie']
san = ['-fsanitize=address,undefined', '-fno-omit-frame-pointer', '-fno-sanitize-recover=all'] if a.sanitize else []
commands = []
env = dict(os.environ, ASAN_OPTIONS='detect_leaks=0:halt_on_error=1', UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=1')

def run(command, log):
    commands.append(command)
    result = subprocess.run(command, text=True, capture_output=True, env=env, timeout=90)
    (o / log).write_text(result.stdout + result.stderr)
    assert result.returncode == 0, (command, result.stdout, result.stderr)
    return result

if not a.helpers_only:
    for mobile in [False, True]:
        mode = 'android' if mobile else 'native'
        mode_flags = ['-DNV_ANDROID=1', '-DNV_TOUCH_UI=1'] if mobile else []
        run(['cc', *flags, *mode_flags, '-fsyntax-only', str(s / 'src/detail.c')], mode + '-syntax.log')
objects = []
for index, path in enumerate([r / 'detail-probe.c', generated]):
    obj = o / (str(index) + '.o')
    run(['cc', *flags, *san, '-c', str(path), '-o', str(obj)], str(index) + '-compile.log')
    objects.append(str(obj))
binary = o / 'native'
run(['cc', *objects, *san, *(['-Wl,-dead_strip'] if darwin else ['-Wl,--gc-sections', '-no-pie']),
     '-pthread', '-lm', '-o', str(binary)], 'link.log')
result = run([str(binary)], 'native.log')
(o / 'native-compile.log').write_text('\n'.join((o / n).read_text() for n in ['0-compile.log', '1-compile.log', 'link.log']))
assert 'private-art-trace:' in result.stdout and 'checks passed' in result.stdout
assert source_hashes == {n: sha(s / n) for n in paths}, 'source mutated during probe'
receipt = {
    'success': True,
    'source_commit': subprocess.check_output(['git', '-C', str(s), 'rev-parse', 'HEAD'], text=True).strip(),
    'source_hashes': source_hashes,
    'fixture_sha256': {n: sha(r / n) for n in ['detail-probe.c', 'cache-boundary.c', 'baseline.json']},
    'runner_sha256': sha(Path(__file__)),
    'sanitize': a.sanitize,
    'macro_off_production_unchanged': True,
    'native_and_android_detail_syntax_passed': not a.helpers_only,
    'full_detail_syntax_skipped': a.helpers_only,
    'runs': [{'mode': 'native', 'binary': str(binary), 'binary_sha256': sha(binary), 'stdout': result.stdout,
              'generated_detail_trace_sha256': sha(trace),
              'generated_cache_source_sha256': sha(generated), 'commands': commands,
              'logs_sha256': {p.name: sha(p) for p in sorted(o.glob('*.log'))}}],
    'scope': 'Exact guarded detail.c trace block and production cache snapshot/hash/lookup bodies. Typed selected-index/catalog/Settings/SDL/cache/cap boundaries. Actual mutex snapshot coherence, ready cache-hit trace, decode/upload upgrade, sourceW unknown, URL redaction, title hashes, selection reset, throttle and 16-event cap. Complete native/Android detail syntax checked separately; no instrumented full-detail/full-cache translation-unit, GL/network or device claim.',
}
(o / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
print(result.stdout, end='')
