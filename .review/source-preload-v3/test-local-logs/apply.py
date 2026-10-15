#!/usr/bin/env python3
"""Apply local log export to an isolated delivery copy, never the PR sources."""
import argparse, hashlib, json
from pathlib import Path

p=argparse.ArgumentParser();p.add_argument('--root',type=Path,required=True);a=p.parse_args()
root=a.root.resolve();here=Path(__file__).resolve().parent;changed=[]
if root in (Path('/workspace/nuvio-source-preload'),Path('/workspace/nuvio-source-preload-combined')):
    raise SystemExit('An isolated delivery copy is required')

def edit(path,old,new):
    f=root/path;s=f.read_text();assert s.count(old)==1,(path,s.count(old));f.write_text(s.replace(old,new));changed.append(path)

gradle='android/app/build.gradle.kts'
edit(gradle,'val raiz = rootProject.projectDir.parentFile',
    'val testLocalLogs = (findProperty("nuvio.testLocalLogs") as String?) == "true"\nval raiz = rootProject.projectDir.parentFile')
edit(gradle,'    defaultConfig {','    buildFeatures { buildConfig = true }\n\n    defaultConfig {\n        buildConfigField("boolean", "NV_TEST_LOCAL_LOGS", testLocalLogs.toString())')
edit(gradle,'                arguments += "-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON"',
    '                arguments += "-DANDROID_SUPPORT_FLEXIBLE_PAGE_SIZES=ON"\n                arguments += "-DNV_TEST_LOCAL_LOGS=${if (testLocalLogs) "ON" else "OFF"}"')
edit('android/app/src/main/cpp/CMakeLists.txt','target_compile_definitions(main PRIVATE NV_ANDROID NV_TOUCH_UI)',
    'target_compile_definitions(main PRIVATE NV_ANDROID NV_TOUCH_UI)\noption(NV_TEST_LOCAL_LOGS "Isolated test APK: local log files only" OFF)\nif(NV_TEST_LOCAL_LOGS)\n  target_compile_definitions(main PRIVATE NV_TEST_LOCAL_LOGS=1)\nendif()')

activity='android/app/src/main/java/space/nuvio/nativelegacy/NuvioActivity.kt'
edit(activity,'    private var vigia: ArranqueVigia? = null', '''    private var vigia: ArranqueVigia? = null
    private val testLocalLogs by lazy { TestLocalLogs(this) }

    // Called from SDL through src/android.c in the isolated diagnostic APK.
    fun salvarRegistroLocal() {
        if (!BuildConfig.NV_TEST_LOCAL_LOGS) return
        runOnUiThread { testLocalLogs.open() }
    }''')
edit(activity,'    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {',
    '    override fun onActivityResult(requestCode: Int, resultCode: Int, data: Intent?) {\n        if (BuildConfig.NV_TEST_LOCAL_LOGS && testLocalLogs.handleResult(requestCode, resultCode, data)) return')
edit(activity,'if (!instalando && !esperandoTela && !isChangingConfigurations) aplicarIconePendente()',
    'if (!instalando && !esperandoTela && !isChangingConfigurations &&\n            !(BuildConfig.NV_TEST_LOCAL_LOGS && testLocalLogs.isChoosing)) aplicarIconePendente()')
edit(activity,'    override fun onDestroy() {',
    '    override fun onDestroy() {\n        if (BuildConfig.NV_TEST_LOCAL_LOGS) testLocalLogs.close()')
watchdog='android/app/src/main/java/space/nuvio/nativelegacy/ArranqueVigia.kt'
edit(watchdog,'.setPositiveButton(t("Enviar registro", "Send log"), null)',
    '.setPositiveButton(if (BuildConfig.NV_TEST_LOCAL_LOGS) "Test logs" else t("Enviar registro", "Send log"), null)')
edit(watchdog,'    private fun enviar(motivo: String, linha: String) {',
    '    private fun enviar(motivo: String, linha: String) {\n        if (BuildConfig.NV_TEST_LOCAL_LOGS) { act.salvarRegistroLocal(); return }')
dest=root/'android/app/src/main/java/space/nuvio/nativelegacy/TestLocalLogs.kt'
dest.write_bytes((here/'TestLocalLogs.kt').read_bytes());changed.append(str(dest.relative_to(root)))
edit('android/app/src/main/res/xml/caminhos_atualizacao.xml', '</paths>',
    '    <!-- Isolated test APK: only redacted user-selected diagnostic snapshots. -->\n    <cache-path name="test_logs" path="test-local-logs/" />\n</paths>')

edit('src/android.h','void android_iniciar(void);','void android_iniciar(void);\n#ifdef NV_TEST_LOCAL_LOGS\nvoid android_salvar_registro_local(void);\n#endif')
edit('src/android.c','void android_interface_modo(int mobile) {', '''#ifdef NV_TEST_LOCAL_LOGS
void android_salvar_registro_local(void) {
  JNIEnv *env = (JNIEnv *)SDL_AndroidGetJNIEnv();
  if (!env) return;
  jobject act = (jobject)SDL_AndroidGetActivity();
  if (!act) return;
  jclass cls = (*env)->GetObjectClass(env, act);
  if (cls) {
    jmethodID metodo = (*env)->GetMethodID(env, cls, "salvarRegistroLocal", "()V");
    if (metodo) (*env)->CallVoidMethod(env, act, metodo);
    (*env)->DeleteLocalRef(env, cls);
  }
  if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
  (*env)->DeleteLocalRef(env, act);
}
#endif
void android_interface_modo(int mobile) {''')
edit('src/android.c','  return (*env)->NewStringUTF(env, NV_REC_URL);',
    '#ifdef NV_TEST_LOCAL_LOGS\n  return (*env)->NewStringUTF(env, "");\n#else\n  return (*env)->NewStringUTF(env, NV_REC_URL);\n#endif')

edit('src/avisos.c','#ifndef NV_REC_URL\n#define NV_REC_URL ""\n#endif', '''#ifndef NV_REC_URL
#define NV_REC_URL ""
#endif
#ifdef NV_TEST_LOCAL_LOGS
#include "android.h"
#undef NV_REC_URL
#define NV_REC_URL ""
#endif''')
edit('src/avisos.c','static void *enviarRegistro(void *u) {',
    'static void *enviarRegistro(void *u) {\n#ifdef NV_TEST_LOCAL_LOGS\n  (void)u; return NULL;\n#endif')
edit('src/avisos.c','void avisos_envio_auto_passo(Uint32 agora) {',
    'void avisos_envio_auto_passo(Uint32 agora) {\n#ifdef NV_TEST_LOCAL_LOGS\n  (void)agora; return;\n#endif')
edit('src/avisos.c','void avisos_enviar_registro_atual(void) {',
    'void avisos_enviar_registro_atual(void) {\n#ifdef NV_TEST_LOCAL_LOGS\n  fflush(stdout); android_salvar_registro_local(); return;\n#endif')
edit('src/avisos.c','static void enviarAgora(void) {',
    'static void enviarAgora(void) {\n#ifdef NV_TEST_LOCAL_LOGS\n  fflush(stdout); android_salvar_registro_local(); return;\n#endif')
edit('src/avisos.c','    case AV_CRASH:\n      if (envioEstado == 0 || envioEstado == 3)',
    '    case AV_CRASH:\n#ifdef NV_TEST_LOCAL_LOGS\n      fflush(stdout); android_salvar_registro_local(); return 0;\n#endif\n      if (envioEstado == 0 || envioEstado == 3)')
edit('src/registro.c','#include "registro.h"','#include "registro.h"\n#ifdef NV_TEST_LOCAL_LOGS\n#include "android.h"\n#endif')
edit('src/registro_envio.inc','void registro_envio_abrir(void) {',
    'void registro_envio_abrir(void) {\n#ifdef NV_TEST_LOCAL_LOGS\n  fflush(stdout); android_salvar_registro_local(); return;\n#endif')
edit('src/ajustes.c','int  ajustes_envio_auto(void)         { return lig(AJ_ENVIO_AUTO); }',
    'int  ajustes_envio_auto(void)         {\n#ifdef NV_TEST_LOCAL_LOGS\n  return 0;\n#else\n  return lig(AJ_ENVIO_AUTO);\n#endif\n}')
edit('src/ajustes.c','static const char *rotuloOpcao(int op) {',
    'static const char *rotuloOpcao(int op) {\n#ifdef NV_TEST_LOCAL_LOGS\n  if (op == AJ_ENVIAR_LOG) return "Test logs";\n  if (op == AJ_ENVIO_AUTO) return "Automatic log upload (disabled for testing)";\n#endif')
edit('src/ajustes.c','static int inativa(int op) {',
    'static int inativa(int op) {\n#ifdef NV_TEST_LOCAL_LOGS\n  if (op == AJ_ENVIO_AUTO) return 1;\n#endif')
edit('src/ajustes.c','  if (op == AJ_ENVIAR_LOG) {\n    static char b[48];',
    '  if (op == AJ_ENVIAR_LOG) {\n#ifdef NV_TEST_LOCAL_LOGS\n    return "View / share / save";\n#endif\n    static char b[48];')
edit('src/ajustes.c','static const char *ajudaOpcao(int op) {',
    'static const char *ajudaOpcao(int op) {\n#ifdef NV_TEST_LOCAL_LOGS\n  if (op == AJ_ENVIAR_LOG) return "View the current and previous session logs, share a redacted file through an app you choose, or save it on your phone.";\n  if (op == AJ_ENVIO_AUTO) return "This test build keeps logs on your phone. It does not upload them to the app maintainer.";\n#endif')
edit('src/ajustes_ux_ilha.inc','      y += aj2Meta(i18n("Destino"), env.disponivel ? i18n("servidor de registros do app") : i18n("indisponível nesta versão"), x, y, w, 1);',
    '#ifdef NV_TEST_LOCAL_LOGS\n      y += aj2Meta(i18n("Destino"), "File on this phone", x, y, w, 1);\n#else\n      y += aj2Meta(i18n("Destino"), env.disponivel ? i18n("servidor de registros do app") : i18n("indisponível nesta versão"), x, y, w, 1);\n#endif')

proof={'test_only':True,'compile_guard':'NV_TEST_LOCAL_LOGS','property':'nuvio.testLocalLogs',
       'paths':[{'path':f,'sha256':hashlib.sha256((root/f).read_bytes()).hexdigest()} for f in sorted(set(changed))]}
(root/'test-local-logs-overlay.json').write_text(json.dumps(proof,indent=2)+'\n');print(json.dumps(proof))
