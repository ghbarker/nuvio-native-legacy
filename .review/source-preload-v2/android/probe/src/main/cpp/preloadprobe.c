#include <jni.h>
#include <stdatomic.h>
static atomic_int calls, ready, errors, resume_generation, resume_accepted, unexpected, frames;
JNIEXPORT void JNICALL Java_space_nuvio_nativelegacy_NvPlayer_nativeIniciar(JNIEnv * env, jclass klass) {  }
JNIEXPORT void JNICALL Java_space_nuvio_nativelegacy_NvPlayer_nativeEvento(JNIEnv * env, jclass klass, jint tipo, jint a, jint b) { atomic_fetch_add(&calls, 1); if (tipo == 1 || tipo == 4 || tipo == 5 || tipo == 9) atomic_fetch_add(&unexpected, 1); if (tipo == 8) atomic_fetch_add(&frames, 1); if (tipo == 1) atomic_fetch_add(&ready, 1); if (tipo == 5) atomic_fetch_add(&errors, 1); }
JNIEXPORT void JNICALL Java_space_nuvio_nativelegacy_NvPlayer_nativeFaixa(JNIEnv * env, jclass klass, jint tipo, jint idx, jstring lingua, jint flags, jstring nome, jstring mime, jint canais) { atomic_fetch_add(&calls, 1); atomic_fetch_add(&unexpected, 1); }
JNIEXPORT void JNICALL Java_space_nuvio_nativelegacy_NvPlayer_nativeFaixasFim(JNIEnv * env, jclass klass, jint selAudio, jint selLeg) { atomic_fetch_add(&calls, 1); atomic_fetch_add(&unexpected, 1); }
JNIEXPORT void JNICALL Java_space_nuvio_nativelegacy_NvPlayer_nativeLegenda(JNIEnv * env, jclass klass, jstring texto, jint durMs) { atomic_fetch_add(&calls, 1); atomic_fetch_add(&unexpected, 1); }
JNIEXPORT void JNICALL Java_space_nuvio_nativelegacy_NvPlayer_nativePos(JNIEnv * env, jclass klass, jint ms) { atomic_fetch_add(&calls, 1); }
JNIEXPORT void JNICALL Java_space_nuvio_nativelegacy_NvPlayer_nativeDecoder4k(JNIEnv * env, jclass klass, jint hevc, jint avc, jint vp9, jint av1) {  }
JNIEXPORT void JNICALL Java_space_nuvio_nativelegacy_NvPlayer_nativeTela(JNIEnv * env, jclass klass, jint hdr, jint dv) {  }
JNIEXPORT void JNICALL Java_space_nuvio_nativelegacy_NvPlayer_nativeHdr(JNIEnv * env, jclass klass, jstring hdr, jint dv, jint atmos) { atomic_fetch_add(&calls, 1); atomic_fetch_add(&unexpected, 1); }
JNIEXPORT void JNICALL Java_space_nuvio_nativelegacy_NvPlayer_nativeErroTexto(JNIEnv * env, jclass klass, jstring texto) { atomic_fetch_add(&calls, 1); atomic_fetch_add(&unexpected, 1); }
JNIEXPORT void JNICALL Java_space_nuvio_nativelegacy_NvPlayer_nativeRetomada(JNIEnv * env, jclass klass, jint geracao, jint aceita) { atomic_fetch_add(&calls, 1); atomic_fetch_add(&unexpected, 1); atomic_store(&resume_generation, geracao); atomic_store(&resume_accepted, aceita); }
JNIEXPORT void JNICALL Java_space_nuvio_nativelegacy_NvPlayer_nativeFitPassiva(JNIEnv * env, jclass klass, jlong rede, jint geracao, jstring origem, jintArray kbps, jlong fimMs) { atomic_fetch_add(&calls, 1); }
JNIEXPORT void JNICALL Java_space_nuvio_nativelegacy_NvPlayer_nativeAudioPcm(JNIEnv * env, jclass klass, jshortArray pcm, jint n, jlong ptsUs) { atomic_fetch_add(&calls, 1); }
JNIEXPORT void JNICALL Java_space_nuvio_nativelegacy_NvPlayer_nativeAudioEstado(JNIEnv * env, jclass klass, jint fmt) { atomic_fetch_add(&calls, 1); atomic_fetch_add(&unexpected, 1); }
JNIEXPORT void JNICALL Java_space_nuvio_nativelegacy_NvPlayer_nativeStats(JNIEnv * env, jclass klass, jlong bytes, jint bufferMs, jint alvoMs, jint conexoes, jint primeiroByteMs, jlong perdidos, jlong quadros, jstring host) { atomic_fetch_add(&calls, 1); }
JNIEXPORT void JNICALL Java_space_nuvio_nativelegacy_NvPlayer_nativeCache(JNIEnv * env, jclass klass, jint estado, jint pedidoMb, jint limiteMb, jint usadoMb) { atomic_fetch_add(&calls, 1); atomic_fetch_add(&unexpected, 1); }
JNIEXPORT void JNICALL Java_space_nuvio_preloadtest_TestProbe_reset(JNIEnv *env, jobject self) { atomic_store(&calls,0); atomic_store(&unexpected,0); atomic_store(&frames,0); atomic_store(&ready,0); atomic_store(&errors,0); atomic_store(&resume_generation,0); atomic_store(&resume_accepted,0); }
JNIEXPORT jint JNICALL Java_space_nuvio_preloadtest_TestProbe_count(JNIEnv *env, jobject self) { return atomic_load(&calls); }
JNIEXPORT jint JNICALL Java_space_nuvio_preloadtest_TestProbe_ready(JNIEnv *env, jobject self) { return atomic_load(&ready); }
JNIEXPORT jint JNICALL Java_space_nuvio_preloadtest_TestProbe_errors(JNIEnv *env, jobject self) { return atomic_load(&errors); }
JNIEXPORT jint JNICALL Java_space_nuvio_preloadtest_TestProbe_resumeGeneration(JNIEnv *env, jobject self) { return atomic_load(&resume_generation); }
JNIEXPORT jint JNICALL Java_space_nuvio_preloadtest_TestProbe_resumeAccepted(JNIEnv *env, jobject self) { return atomic_load(&resume_accepted); }
JNIEXPORT jint JNICALL Java_space_nuvio_preloadtest_TestProbe_unexpected(JNIEnv *env, jobject self) { return atomic_load(&unexpected); }
JNIEXPORT jint JNICALL Java_space_nuvio_preloadtest_TestProbe_frames(JNIEnv *env, jobject self) { return atomic_load(&frames); }
