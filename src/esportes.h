#ifndef NV_ESPORTES_H
#define NV_ESPORTES_H
#include <stddef.h>
#include <time.h>

#define ESP_MAX_LIGAS 1024
#define ESP_MAX_EVENTOS 400
#define ESP_MAX_PARTICIPANTES 2
#define ESP_MAX_EMISSORAS 8
typedef struct { const char *id, *nome; } EspModalidade;
typedef struct { char id[80], nome[160], logo[512]; } EspLiga;
typedef struct {
  char id[80], nome[120], curto[100], sigla[20], logo[512], pontos[32];
} EspParticipante;
typedef struct {
  char id[100], nome[256], liga[160], ligaId[80], modalidade[32];
  char local[160], detalhe[120], sessao[48];
  time_t inicio, fim;
  int estado; /* 0 scheduled, 1 live, 2 finished, 3 postponed/cancelled */
  int nParticipantes, nEmissoras;
  EspParticipante participantes[ESP_MAX_PARTICIPANTES];
  char emissoras[ESP_MAX_EMISSORAS][80];
} EspEvento;

int esportes_modalidades_n(void);
const EspModalidade *esportes_modalidade(int i);
/* Parsing/matching are pure: no requests and no access to the player. */
int esportes_ligas_ler(const char *json, EspLiga *out, int cap);
int esportes_eventos_ler(const char *json, const char *modalidade,
                        const EspLiga *liga, EspEvento *out, int cap);
/* Only participants/event identity AND a plausible time window confirm a
 * match. League or broadcaster affiliation on its own never confirms it. */
int esportes_corresponde(const EspEvento *ev, const char *titulo,
                         time_t inicio, time_t fim);
int esportes_emissora(const EspEvento *ev, const char *nome);

/* UI-thread ownership; background jobs publish by generation. */
void esportes_abrir_sessao(void);
int esportes_habilitado(int modalidade);
int esportes_habilitar(int modalidade, int ligado);
int esportes_favorito(const EspEvento *ev, int participante);
int esportes_alternar_favorito(const EspEvento *ev, int participante);
void esportes_pedir_ligas(int modalidade, int atualizar);
void esportes_pedir_eventos(int modalidade, const EspLiga *liga, int atualizar);
void esportes_passo(void);
void esportes_cancelar(void);
int esportes_carregando(void);
int esportes_erro(void);
unsigned esportes_revisao(void);
int esportes_ligas_n(void);
const EspLiga *esportes_liga(int i);
int esportes_eventos_n(void);
const EspEvento *esportes_evento(int i);
#endif
