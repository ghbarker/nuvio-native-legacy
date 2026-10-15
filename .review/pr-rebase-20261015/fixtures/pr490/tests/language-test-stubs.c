// Unrelated account-language callbacks retained by ASan's global registration.
// These fixtures exercise watched settings and never update account languages.
__attribute__((weak)) void ling_conta_legenda(const char *v) { (void)v; }
__attribute__((weak)) void ling_conta_legenda2(const char *v) { (void)v; }
__attribute__((weak)) void ling_conta_audio(const char *v) { (void)v; }
