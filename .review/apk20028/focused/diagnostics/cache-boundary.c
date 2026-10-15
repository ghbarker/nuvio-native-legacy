// Attached after the exact production Item/hash/lookup/snapshot definitions.
static SDL_Surface decoded;
void probe_cache(const char *url, int state, int texture, int request, int used,
                 int variant, int sourceWidth, int gpuWidth, int gpuHeight,
                 int decodedWidth, int decodedHeight) {
  SDL_LockMutex(mtx);
  memset(itens, 0, sizeof itens);
  Item *it = itens;
  snprintf(it->caminho, sizeof it->caminho, "%s", url);
  it->hash = hashCaminho(url); it->estado = state; it->tex = texture;
  it->limite = request; it->tetoUsado = used; it->limiteTamanho = variant;
  it->fonteW = sourceWidth; it->w = gpuWidth; it->h = gpuHeight;
  decoded.w = decodedWidth; decoded.h = decodedHeight;
  if (decodedWidth) it->sup = &decoded;
  SDL_UnlockMutex(mtx);
}
void probe_absent(void) {
  SDL_LockMutex(mtx); memset(itens, 0, sizeof itens); SDL_UnlockMutex(mtx);
}
unsigned long probe_cache_checksum(void) {
  const unsigned char *p = (const unsigned char *)itens;
  unsigned long h = 2166136261UL;
  SDL_LockMutex(mtx);
  for (size_t i = 0; i < sizeof itens; i++) { h ^= p[i]; h *= 16777619UL; }
  SDL_UnlockMutex(mtx); return h;
}
static void *writer(void *unused) {
  (void)unused;
  for (int i = 1000; i < 3000; i++) {
    SDL_LockMutex(mtx); itens[0].w = i * 2; itens[0].h = i; SDL_UnlockMutex(mtx);
  }
  return NULL;
}
void probe_snapshot_checks(void) {
  TexTesteArte s; memset(&s, 0x7f, sizeof s);
  assert(!tex_teste_arte(NULL, &s) && !s.estado && !s.fonteW && !s.gpuW);
  assert(s.qualidade == 2 && s.heroi == 1920);
  assert(!tex_teste_arte("https://missing.example/SECRET", &s));
  assert(!tex_teste_arte("x", NULL));
  const char *u = "https://image.tmdb.org/t/p/w1280/PRIVATE.jpg";
  probe_cache(u, PRONTO, 77, 1920, 1280, 1280, 0, 1280, 640, 0, 0);
  assert(tex_teste_arte(u, &s) && s.estado == PRONTO && s.textura);
  assert(!s.fonteW && s.gpuW == 1280 && s.decW == 1280);
  pthread_t thread; assert(!pthread_create(&thread, NULL, writer, NULL));
  for (int i = 0; i < 2000; i++) {
    assert(tex_teste_arte(u, &s));
    assert(s.gpuW == s.gpuH * 2 && s.decW == s.gpuW && s.decH == s.gpuH);
  }
  assert(!pthread_join(thread, NULL)); probe_absent();
}
