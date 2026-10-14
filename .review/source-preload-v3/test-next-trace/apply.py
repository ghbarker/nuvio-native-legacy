#!/usr/bin/env python3
"""Delivery-only diagnostic overlay. Never applied to the PR source tree."""
import argparse
import hashlib
import json
from pathlib import Path

p = argparse.ArgumentParser()
p.add_argument('--root', type=Path, required=True)
a = p.parse_args()
root = a.root.resolve()
if root in (Path('/workspace/nuvio-source-preload'), Path('/workspace/nuvio-source-preload-combined')):
    raise SystemExit('Apply only to an isolated delivery copy, never a product tree')
records = []

def replace(path, old, new):
    f = root / path
    text = f.read_text()
    if text.count(old) != 1:
        raise SystemExit(f'{path}: expected one strict anchor, got {text.count(old)}')
    f.write_text(text.replace(old, new))
    records.append(path)

replace('src/posplay.c', 'int posplay_pediu_episodio(int *t, int *e) {\n  if (!pedT) return 0;', '''int posplay_pediu_episodio(int *t, int *e) {
  if (!pedT) return 0;
#ifdef NV_TEST_LOCAL_LOGS
  printf("[test-next] event=request t=%d e=%d automatic=%d ms=%u\\n", pedT, pedE, pedAutomatico, SDL_GetTicks());
#endif''')

replace('src/posplay.c', '  k = e->key.keysym.sym;\n', '''  k = e->key.keysym.sym;
#ifdef NV_TEST_LOCAL_LOGS
  printf("[test-next] event=control key=%d focus=%d scene=%d series=%d next=%d:%d ms=%u\\n",
    k, foco, aguardaCena, serie, proxT, proxE, SDL_GetTicks());
#endif
''')

replace('src/posplay.c', '  // A contagem so vale para o proximo episodio.\n', '''#ifdef NV_TEST_LOCAL_LOGS
  // Test APK only: state changes and a bounded five-second pulse near Next.
  // No source URLs, accounts, API keys or profile names enter these records.
  {
    static unsigned testEstado;
    static Uint32 testQuando;
    static int testT, testE;
    int t = 0, e = 0;
    const CatEp *testProximo = player_proximo_episodio();
    long long testLancamento = prox_episodio_data_ms(testProximo);
    int testDataConhecida = testLancamento != PROX_SEM_DATA;
    int testFutura = testDataConhecida && testLancamento > (long long)time(NULL) * 1000;
    player_episodio_atual(&t, &e);
    unsigned estado = (unsigned)!!visivel | (unsigned)!!fecharEm << 1 |
      (unsigned)!!marcadorReal << 2 | (unsigned)!!aguardaCena << 3 |
      (unsigned)!!aguardaBusca << 4 | (unsigned)!!recuou << 5 |
      (unsigned)!!dispensado << 6 | (unsigned)!!ajustes_proximo_auto() << 7 |
      (unsigned)!!ajustes_pular_creditos() << 8 | (unsigned)!!janelaSerie << 9 |
      (unsigned)!!preVideoTentou << 10 | (unsigned)!!preVideoRenovou << 11 |
      (unsigned)!!pedT << 12 | (unsigned)!!video_terminou() << 13 |
      (unsigned)!!video_pronto() << 14 | (unsigned)!!video_bufferando_ms() << 15 |
      (unsigned)!!ajustes_fonte_precarregar() << 16 | (unsigned)!!ajustes_fonte_preparar() << 17 |
      (unsigned)!!ajustes_fonte_manual() << 18 | (unsigned)!!testDataConhecida << 19 |
      (unsigned)!!testFutura << 20;
    int perto = visivel || janelaSerie || preVideoTentou || aguardaBusca;
    if (ehSerie && (estado != testEstado || t != testT || e != testE ||
                    (perto && agora - testQuando >= 5000u))) {
      printf("[test-next] event=state ms=%u current=%d:%d next=%d:%d pos=%.2f native_pos=%.2f dur=%.2f ended=%d ready=%d buffering_ms=%d rate=%d marker=%.2f real=%d window=%d visible=%d auto=%d skip=%d scene=%d seek=%d rewind=%d dismissed=%d countdown_ms=%u preload=%d refreshed=%d preload_enabled=%d prepare_enabled=%d manual=%d release_known=%d release_future=%d release_ms=%lld\\n",
        agora, t, e, proxT, proxE, posSeg, video_pos(), durSeg, video_terminou(), video_pronto(),
        video_bufferando_ms(), player_velocidade_efetiva(), creditosSeg,
        marcadorReal, janelaSerie, visivel, ajustes_proximo_auto(), ajustes_pular_creditos(),
        aguardaCena, aguardaBusca, recuou, dispensado,
        fecharEm && fecharEm > agora ? fecharEm - agora : 0u, preVideoTentou, preVideoRenovou,
        ajustes_fonte_precarregar(), ajustes_fonte_preparar(), ajustes_fonte_manual(),
        testDataConhecida, testFutura, testDataConhecida ? testLancamento : 0LL);
      testEstado = estado; testQuando = agora; testT = t; testE = e;
    }
  }
#endif
  // A contagem so vale para o proximo episodio.
''')

replace('src/app.c', '  int pronta = proximopreparo_consumir(alvo, &preparada);\n', '''  int pronta = proximopreparo_consumir(alvo, &preparada);
#ifdef NV_TEST_LOCAL_LOGS
  printf("[test-next] event=handoff t=%d e=%d target_valid=%d prepared=%d pending=%d selected=%d actor_ok=%d owner_ok=%d ms=%u\\n",
    t, e, !!alvo[0], pronta, proximopreparo_em_curso(alvo), pronta ? preparada.indiceFonte : -1,
    perfis_sessao_atual(sessaoPerfil), !pronta || preparada.sessaoPerfil == sessaoPerfil, SDL_GetTicks());
#endif
''')
replace('src/app.c', '    proximoIntento.tem = 1; proximoIntento.titulo = titulo;\n', '''#ifdef NV_TEST_LOCAL_LOGS
    if (!proximoIntento.tem)
      printf("[test-next] event=wait_preparation t=%d e=%d ms=%u\\n", t, e, SDL_GetTicks());
#endif
    proximoIntento.tem = 1; proximoIntento.titulo = titulo;
''')
replace('src/app.c', '  player_definir_episodio(t, e);\n  // Abrir/fechar nao seguram', '''  player_definir_episodio(t, e);
#ifdef NV_TEST_LOCAL_LOGS
  printf("[test-next] event=episode_opened t=%d e=%d prepared=%d ms=%u\\n", t, e, pronta, SDL_GetTicks());
#endif
  // Abrir/fechar nao seguram''')
replace('src/app.c', '    memset(&proximoIntento, 0, sizeof proximoIntento); proximopreparo_cancelar();\n  } else if (!proximopreparo_em_curso', '''#ifdef NV_TEST_LOCAL_LOGS
    printf("[test-next] event=cancel_pending player=%d exiting=%d episode_changed=%d title_changed=%d generation_changed=%d actor_changed=%d ms=%u\\n",
      player_aberto(), player_quer_sair(), !atual[0] || strcmp(atual, proximoIntento.atual) != 0,
      player_indice() != proximoIntento.titulo, proximoIntento.geracao != proximopreparo_geracao(),
      !perfis_sessao_atual(proximoIntento.sessaoPerfil), SDL_GetTicks());
#endif
    memset(&proximoIntento, 0, sizeof proximoIntento); proximopreparo_cancelar();
  } else if (!proximopreparo_em_curso''')
replace('src/app.c', '  { int t = 0, e = 0;\n    if (aguardandoFonte != 2 && posplay_pediu_episodio', '''  { int t = 0, e = 0;
#ifdef NV_TEST_LOCAL_LOGS
    static int testGuarda = -1;
    if (testGuarda != aguardandoFonte) {
      printf("[test-next] event=source_guard state=%d player=%d ms=%u\\n", aguardandoFonte, player_aberto(), SDL_GetTicks());
      testGuarda = aguardandoFonte;
    }
#endif
    if (aguardandoFonte != 2 && posplay_pediu_episodio''')

result = {'test_only': True, 'compile_guard': 'NV_TEST_LOCAL_LOGS',
          'changes': [{'path': f, 'sha256': hashlib.sha256((root / f).read_bytes()).hexdigest()}
                      for f in sorted(set(records))]}
(root / 'test-next-trace-overlay.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result))
