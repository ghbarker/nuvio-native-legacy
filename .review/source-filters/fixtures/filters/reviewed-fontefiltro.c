#include "fontefiltro.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint64_t b(int i) { return UINT64_C(1) << i; }
static FonteFiltroDados dados(FonteFiltroEntrada e) {
  FonteFiltroDados d; fontefiltro_classificar(&e,&d); return d;
}

static void taxonomia(void) {
  const int contagem[] = {8,13,16,13,5,6,15};
  for (int c = 0; c < FONTEFILTRO_N; c++) {
    assert(fontefiltro_opcoes_n(c) == contagem[c]);
    assert(*fontefiltro_rotulo(c));
    for (int i = 0; i < contagem[c]; i++) {
      assert(*fontefiltro_opcao(c,i)); assert(fontefiltro_bit(c,i) == b(i));
    }
    assert(fontefiltro_desconhecido(c) == b(contagem[c]-1));
    assert(fontefiltro_valores(c,"Unknown") == 0);
  }
  assert(fontefiltro_opcoes_n(-1) == 0);
  assert(!*fontefiltro_opcao(FONTEFILTRO_N,0));
  assert(fontefiltro_bit(0,80) == 0);
  const char *res[] = {"2160p","1440p","1080p","720p","576p","480p","360p"};
  for (int i = 0; i < 7; i++) assert(fontefiltro_valores(FONTEFILTRO_RESOLUCAO,res[i]) == b(i));
  const char *qual[] = {"BluRay REMUX","BluRay","WEB-DL","WEBRip","HDRip","HC HD-Rip","DVDRip","HDTV","CAM","TS","TC","SCR"};
  for (int i = 0; i < 12; i++) assert(fontefiltro_valores(FONTEFILTRO_QUALIDADE,qual[i]) == b(i));
  const char *enc[] = {"AV1","HEVC","AVC","XviD","DivX"};
  for (int i = 0; i < 5; i++) assert(fontefiltro_valores(FONTEFILTRO_ENCODE,enc[i]) == b(i));
  const char *lang[] = {"en","hi","it","es","fr","de","pt","pl","cs","la","ja","ko","zh","multi"};
  for (int i = 0; i < 14; i++) assert(fontefiltro_valores(FONTEFILTRO_LINGUAS,lang[i]) == b(i));
}

static void tags(void) {
  assert(fontefiltro_valores(FONTEFILTRO_ENCODE,"movie.H.265.mkv") == b(1));
  assert(fontefiltro_valores(FONTEFILTRO_ENCODE,"movie.H.264.mkv") == b(2));
  assert(fontefiltro_valores(FONTEFILTRO_ENCODE,"h265x h2640 x265release cavc") == 0);
  assert(fontefiltro_valores(FONTEFILTRO_RESOLUCAO,"DTS-HD MA") == 0);
  assert(fontefiltro_valores(FONTEFILTRO_VISUAL,"DVDRip DVD") == 0);
  assert(fontefiltro_valores(FONTEFILTRO_VISUAL,"HDR10+.DV.HLG.10-bit.3D.IMAX.AI.H-OU.H-SBS") ==
         (b(0)|b(3)|b(4)|b(5)|b(6)|b(7)|b(8)|b(9)|b(10)|b(11)|b(13)|b(14)));
  assert(fontefiltro_valores(FONTEFILTRO_VISUAL,"Dolby Vision") == (b(1)|b(5)));
  assert(fontefiltro_valores(FONTEFILTRO_VISUAL,"HDR Only SDR") == (b(2)|b(6)|b(12)));
  assert(fontefiltro_valores(FONTEFILTRO_AUDIO,"Atmos DD+ DTS:X DTS-HD MA DTS-ES TrueHD OPUS FLAC AAC") ==
         (b(0)|b(1)|b(2)|b(3)|b(4)|b(5)|b(6)|b(7)|b(8)|b(9)|b(10)|b(11)));
  assert(fontefiltro_valores(FONTEFILTRO_AUDIO,"Movie.DDP5.1.AAC2.0.mkv") == (b(1)|b(2)|b(11)));
  assert(fontefiltro_valores(FONTEFILTRO_CANAIS,"Movie.DDP5.1.AAC2.0.mkv") == (b(2)|b(3)));
  assert(fontefiltro_valores(FONTEFILTRO_CANAIS,"7.1/6.1/6ch/2.0") == (b(0)|b(1)|b(2)|b(3)));
  assert(fontefiltro_valores(FONTEFILTRO_AUDIO,"shadowDDplus atmospherics opusland AACX") == 0);
}

static void filtros(void) {
  FonteFiltroConfig cfg = {0}; FonteFiltroDados d = {0};
  assert(fontefiltro_config_validar(&cfg));
  assert(fontefiltro_permite(&cfg,&d));
  for (int c = 0; c < FONTEFILTRO_N; c++) {
    cfg.excluir[c] = b(0);
    assert(fontefiltro_permite(&cfg,&d)); /* Ausencia nao significa excluido. */
    d.valores[c] = b(0); assert(!fontefiltro_permite(&cfg,&d));
    d.valores[c] = b(1); assert(fontefiltro_permite(&cfg,&d));
    d.valores[c] = 0; cfg.excluir[c] = fontefiltro_desconhecido(c);
    assert(!fontefiltro_permite(&cfg,&d));
    d.valores[c] = b(0); assert(fontefiltro_permite(&cfg,&d));
    d.valores[c] = 0; cfg.excluir[c] = 0;
  }
  d.valores[FONTEFILTRO_AUDIO] = b(0)|b(8); cfg.excluir[FONTEFILTRO_AUDIO] = b(8);
  assert(!fontefiltro_permite(&cfg,&d)); /* Um tag anunciado basta. */
  cfg.excluir[FONTEFILTRO_AUDIO] = 0;
  d.valores[FONTEFILTRO_LINGUAS] = b(0)|b(2);
  cfg.excluir[FONTEFILTRO_LINGUAS] = b(2);
  assert(fontefiltro_permite(&cfg,&d)); /* Ingles continua disponivel. */
  cfg.excluir[FONTEFILTRO_LINGUAS] |= b(0);
  assert(!fontefiltro_permite(&cfg,&d));
  cfg.excluir[FONTEFILTRO_LINGUAS] = b(9);
  d = dados((FonteFiltroEntrada){.linguas="es-419"});
  assert(d.valores[FONTEFILTRO_LINGUAS] == b(9));
  assert(!fontefiltro_permite(&cfg,&d));
  d = dados((FonteFiltroEntrada){.linguas="es-la"});
  assert(d.valores[FONTEFILTRO_LINGUAS] == b(9));
  assert(!fontefiltro_permite(&cfg,&d));
  d = dados((FonteFiltroEntrada){.linguas="es es-419"});
  assert(d.valores[FONTEFILTRO_LINGUAS] == (b(3)|b(9)));
  assert(fontefiltro_permite(&cfg,&d));
  d = dados((FonteFiltroEntrada){.linguas="es-ES"});
  assert(d.valores[FONTEFILTRO_LINGUAS] == b(3));
  assert(fontefiltro_permite(&cfg,&d));
  cfg.excluir[FONTEFILTRO_LINGUAS] = 0;
  cfg.maxGB = 10; d.tamanhoBytes = UINT64_C(10000000000);
  assert(fontefiltro_permite(&cfg,&d));
  d.tamanhoBytes++; assert(!fontefiltro_permite(&cfg,&d));
  d.tamanhoBytes = 0; assert(fontefiltro_permite(&cfg,&d));
  cfg.maxGB = 0; d.tamanhoBytes = UINT64_MAX; assert(fontefiltro_permite(&cfg,&d));
  cfg.maxGB = 1001; assert(!fontefiltro_config_validar(&cfg)); cfg.maxGB = 1000;
  assert(fontefiltro_config_validar(&cfg));
  cfg.excluir[0] = b(63); assert(!fontefiltro_config_validar(&cfg)); cfg.excluir[0] = 0;
  strcpy(cfg.grupos,"bad\nvalue"); assert(!fontefiltro_config_validar(&cfg));
  memset(cfg.grupos,'X',sizeof cfg.grupos); assert(!fontefiltro_config_validar(&cfg));
}

static void prioridades(void) {
  FonteFiltroDados d = dados((FonteFiltroEntrada){
    .resolucao="720p",.qualidade="WEBRip",.encode="AVC",.linguas="en",
    .visual="DV",.audio="Atmos",.canais="2.0",.grupo="CUSTOM",
    .filename="Movie.1080p.BluRay.HDR10+.TrueHD.7.1.HEVC.IT-PSA.mkv",.tamanhoBytes=123});
  assert(d.valores[FONTEFILTRO_RESOLUCAO] == b(3));
  assert(d.valores[FONTEFILTRO_QUALIDADE] == b(3));
  assert(d.valores[FONTEFILTRO_ENCODE] == b(2));
  assert(d.valores[FONTEFILTRO_LINGUAS] == b(0));
  assert(d.valores[FONTEFILTRO_VISUAL] == (b(0)|b(3)|b(4)|b(5)|b(6)));
  assert(d.valores[FONTEFILTRO_AUDIO] == (b(0)|b(8)));
  assert(d.valores[FONTEFILTRO_CANAIS] == (b(0)|b(3)));
  assert(!strcmp(d.grupo,"CUSTOM") && d.tamanhoBytes == 123);
  d = dados((FonteFiltroEntrada){.filename="Film.2160p.WEB-DL.H.265.DDP5.1.EN-PSA.mkv"});
  assert(d.valores[FONTEFILTRO_RESOLUCAO] == b(0));
  assert(d.valores[FONTEFILTRO_QUALIDADE] == b(2));
  assert(d.valores[FONTEFILTRO_ENCODE] == b(1));
  assert(d.valores[FONTEFILTRO_LINGUAS] == b(0));
  assert(!strcmp(d.grupo,"PSA"));
  d = dados((FonteFiltroEntrada){.resolucao="Unknown",.filename="Film.2160p.mkv"});
  assert(d.valores[FONTEFILTRO_RESOLUCAO] == 0);
}

static void linguasSeguras(void) {
  FonteFiltroDados d = dados((FonteFiltroEntrada){.filename="It.2017.mkv",.texto="It\nThe English Patient\nSub Eng\nSubtitles: Italian 🇮🇹\nhttps://host/en/movie"});
  assert(d.valores[FONTEFILTRO_LINGUAS] == 0);
  d = dados((FonteFiltroEntrada){.filename="The.English.Patient.1996.1080p.WEB-DL.mkv"});
  assert(d.valores[FONTEFILTRO_LINGUAS] == 0);
  d = dados((FonteFiltroEntrada){.filename="It.2017.1080p.WEB-DL.EN.IT.mkv"});
  assert(d.valores[FONTEFILTRO_LINGUAS] == (b(0)|b(2)));
  d = dados((FonteFiltroEntrada){.filename="Film.[EN/IT].mkv"});
  assert(d.valores[FONTEFILTRO_LINGUAS] == (b(0)|b(2)));
  d = dados((FonteFiltroEntrada){.filename="[English Patient].mkv"});
  assert(d.valores[FONTEFILTRO_LINGUAS] == 0);
  d = dados((FonteFiltroEntrada){.filename="[English.Patient].mkv"});
  assert(d.valores[FONTEFILTRO_LINGUAS] == 0);
  d = dados((FonteFiltroEntrada){.texto="Languages: English, Italian\nSub: French 🇫🇷"});
  assert(d.valores[FONTEFILTRO_LINGUAS] == (b(0)|b(2)));
  d = dados((FonteFiltroEntrada){.texto="🇬🇧 🇮🇹 🇧🇷"});
  assert(d.valores[FONTEFILTRO_LINGUAS] == (b(0)|b(2)|b(6)));
  d = dados((FonteFiltroEntrada){.linguas="eng ita fra ger por cze jpn kor chi multi"});
  assert(d.valores[FONTEFILTRO_LINGUAS] == (b(0)|b(2)|b(4)|b(5)|b(6)|b(8)|b(10)|b(11)|b(12)|b(13)));
  d = dados((FonteFiltroEntrada){.texto="https://cdn.test/HDR.DV.H.265.1080p.DTS-HD/film\nSubtitles: HDR"});
  for (int c = 0; c < FONTEFILTRO_N; c++) assert(d.valores[c] == 0);
}

static void grupos(void) {
  char csv[512] = "unchanged", longo[600];
  assert(fontefiltro_grupos_normalizar(" PSA, YTS.MX ,psa,,Custom_Group-1 ",csv,sizeof csv));
  assert(!strcmp(csv,"PSA,YTS.MX,Custom_Group-1"));
  char antes[512]; strcpy(antes,csv);
  assert(!fontefiltro_grupos_normalizar("bad\nrow",csv,sizeof csv)); assert(!strcmp(csv,antes));
  memset(longo,'a',sizeof longo-1); longo[sizeof longo-1] = 0;
  assert(!fontefiltro_grupos_normalizar(longo,csv,sizeof csv));
  assert(!fontefiltro_grupos_normalizar("valid",csv,3)); assert(!strcmp(csv,antes));
  FonteFiltroConfig cfg = {0}; strcpy(cfg.grupos,"psa,YTS.MX,Custom_Group-1");
  FonteFiltroDados d = dados((FonteFiltroEntrada){.filename="Film.1080p.x265-PSA.mkv"});
  assert(!strcmp(d.grupo,"PSA")); assert(!fontefiltro_permite(&cfg,&d));
  d = dados((FonteFiltroEntrada){.filename="Film.1080p.x264-YTS.MX.mp4"});
  assert(!strcmp(d.grupo,"YTS.MX")); assert(!fontefiltro_permite(&cfg,&d));
  d = dados((FonteFiltroEntrada){.grupo="Custom_Group-1"}); assert(!fontefiltro_permite(&cfg,&d));
  d = dados((FonteFiltroEntrada){.grupo="PSA2"}); assert(fontefiltro_permite(&cfg,&d));
  const char *semgrupo[] = {"Film.1080p.WEB-DL.mkv","Film.1080p.Blu-Ray.mkv","Film.DTS-HD.mkv","Film.H-SBS.mkv","https://host/film-GRP.mkv"};
  for (size_t i = 0; i < sizeof semgrupo / sizeof semgrupo[0]; i++) {
    d = dados((FonteFiltroEntrada){.filename=semgrupo[i]}); assert(!d.grupo[0]);
  }
  d = dados((FonteFiltroEntrada){.filename="/downloads/Film.2024.1080p.WEBRip.[YTS.MX].mp4"});
  assert(!strcmp(d.grupo,"YTS.MX")); assert(!fontefiltro_permite(&cfg,&d));
  const char *colchetes[] = {"Film.1080p.WEBRip.[1080p].mkv","Film.1080p.WEBRip.[ENG].mkv","Film.1080p.WEBRip.[HDR].mkv","Film.1080p.WEBRip.[2024].mkv"};
  for (size_t i = 0; i < sizeof colchetes / sizeof colchetes[0]; i++) {
    d = dados((FonteFiltroEntrada){.filename=colchetes[i]}); assert(!d.grupo[0]);
  }
}

static void torrentio(void) {
  /* The typical infoHash-only addon response puts its release name in title,
     without any native resolver metadata or behaviorHints.filename. */
  FonteFiltroDados d = dados((FonteFiltroEntrada){.texto="Movie.2024.1080p.WEB-DL.DDP5.1.ENG-SPARKS\n👤 24 💾 2.5 GB\nTorrentio"});
  assert(!strcmp(d.grupo,"SPARKS"));
  assert(d.valores[FONTEFILTRO_LINGUAS] == b(0));
  assert(d.valores[FONTEFILTRO_QUALIDADE] == b(2));
  FonteFiltroConfig cfg = {0}; strcpy(cfg.grupos,"sparks");
  assert(!fontefiltro_permite(&cfg,&d));
  memset(cfg.grupos,0,sizeof cfg.grupos); cfg.excluir[FONTEFILTRO_LINGUAS] = b(0);
  assert(!fontefiltro_permite(&cfg,&d));
  d = dados((FonteFiltroEntrada){.texto="It\nThe English Patient\nCast: EN-SPARKS 1080p HEVC\nProvider: EN-OTHER 1080p WEB-DL\nSource: ENG-CUSTOM 1080p x265\nSub: Movie.2024.1080p.WEB-DL.FR-GROUP\nhttps://host/Movie.2024.1080p.WEB-DL.IT-URL"});
  assert(!d.grupo[0] && !d.valores[FONTEFILTRO_LINGUAS]);
  d = dados((FonteFiltroEntrada){.texto="Film.2024.1080p.WEBRip.ENG.[YTS.MX].mp4\n👤 12"});
  assert(!strcmp(d.grupo,"YTS.MX") && d.valores[FONTEFILTRO_LINGUAS] == b(0));
  d = dados((FonteFiltroEntrada){.filename="Movie.2024.1080p.ts"});
  assert(!d.valores[FONTEFILTRO_QUALIDADE]);
  d = dados((FonteFiltroEntrada){.texto="Movie.2024.1080p.ts\n👤 3"});
  assert(!d.valores[FONTEFILTRO_QUALIDADE]);
  d = dados((FonteFiltroEntrada){.qualidade="TS",.filename="Movie.2024.1080p.ts"});
  assert(d.valores[FONTEFILTRO_QUALIDADE] == b(9));
  d = dados((FonteFiltroEntrada){.filename="Movie.2024.TS.1080p.ts"});
  assert(d.valores[FONTEFILTRO_QUALIDADE] == b(9));
}

int main(void) {
  taxonomia(); tags(); filtros(); prioridades(); linguasSeguras(); grupos(); torrentio();
  puts("fontefiltro: taxonomy, exclusion semantics, exact size, metadata, languages and release groups PASS");
  return 0;
}
