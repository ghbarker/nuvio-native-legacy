void cat_republicar_fileiras(const CatFileira *novasFils, int nNovas) {
  int k, q, v = 0;
  // nNovas == 0 e valido (#319): tirar a ultima fileira tem de esvaziar a tela;
  // com o guarda antigo (< 1) a fileira de addon removido ficava publicada.
  if (!novasFils || nNovas < 0 || n < 1) return;
  q = nNovas > CAT_FIL_MAX ? CAT_FIL_MAX : nNovas;
  catTravar();
  // A JANELA QUE ESTA PUBLICADA VENCE A DA MONTAGEM, para a mesma chave. Quem
  // chama (desc_remontar_fileiras) republica o retrato da ultima montagem
  // completa, e ele nao sabe do que mudou depois SEM REDE: um card tirado de
  // "Continuar assistindo" (cat_tirar_continuar) ou uma refacao da fileira
  // (cat_trocar_continuar, que desliza o `ini` das seguintes). Sem isto o
  // proximo sync que remonta as fileiras devolvia n=3 a uma janela que ja era
  // n=2 — o card "voltava" como o primeiro item da fileira de baixo.
  { static CatFileira ajust[CAT_FIL_MAX];
    int r;
    for (k = 0; k < q; k++) {
      ajust[k] = novasFils[k];
      for (r = 0; r < nFils; r++)
        if (!strcmp(fils[r].chave, ajust[k].chave)) {
          ajust[k].ini = fils[r].ini; ajust[k].n = fils[r].n; break;
        }
      // Chave que saiu da lista publicada porque ESVAZIOU (o ultimo card de
      // "Continuar assistindo" tirado) nao volta com a janela velha.
      if (r == nFils && !strcmp(ajust[k].chave, "continue_watching")) ajust[k].n = 0;
    }
    // E a que NASCEU depois da montagem (cat_trocar_continuar cria a fileira
    // quando o primeiro titulo entra em progresso) nao some: entra na frente,
    // onde montar() e cat_trocar_continuar a poem.
    { int cw = fileiraContinuar(), tem = 0;
      for (k = 0; k < q; k++) if (!strcmp(ajust[k].chave, "continue_watching")) tem = 1;
      if (cw >= 0 && !tem && q < CAT_FIL_MAX) {
        memmove(ajust + 1, ajust, sizeof *ajust * (size_t)q);
        ajust[0] = fils[cw];
        q++;
      } }
    novasFils = ajust; }
  // IGUAL AO QUE ESTA: nao mexe. A remontagem sem rede roda a cada sync e a
  // cada ajuste de fileira; quando o resultado e o mesmo conjunto na mesma
  // ordem, zerar e reescrever as fileiras faz a home se reconstruir por nada.
  if (q == nFils) {
    int igual = 1;
    for (k = 0; k < q && igual; k++)
      if (strcmp(novasFils[k].chave, fils[k].chave) ||
          strcmp(novasFils[k].base, fils[k].base) ||
          strcmp(novasFils[k].catId, fils[k].catId) ||
          novasFils[k].ini != fils[k].ini || novasFils[k].n != fils[k].n ||
          novasFils[k].estado != fils[k].estado ||
          novasFils[k].socialGeracao != fils[k].socialGeracao) igual = 0;
    if (igual) { pthread_mutex_unlock(&pubTrava); return; }
  }
  nFils = 0;                 // ver a nota em catalogo.h: zera antes de mexer
  for (k = 0; k < q; k++) {
    CatFileira f = novasFils[k];
    if (f.ini < 0 || f.ini > n || (f.n < 1 && !f.estado)) continue;
    if (f.ini + f.n > n) f.n = n - f.ini;
    fils[v++] = f;
  }
  nFils = v;
  // BUMPA A REVISAO sempre que as fileiras mudaram. cat_definir_tudo e
  // cat_trocar_continuar ja fazem isto; sem isto aqui, cat_republicar_fileiras
  // (usada por desc_remontar_fileiras na mudanca de ordem/colecao/limite sem
  // rede) trocava fils[] sem avisar ninguem — e a home so percebia na proxima
  // publicacao da descoberta. Agora cat_revisao() e um guarda correto do
  // estado das fileiras, usado por sincronizarFileiras em home.c.
  revisaoSobe(); mudou();
  pthread_mutex_unlock(&pubTrava);
}
