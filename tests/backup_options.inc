// Deliberate Nuvio-owned exclusions, checked against the web sync schema.
// New scalar choices must either have a real native backup owner or be added
// here after verifying that the account already synchronizes them.
static void conferirCobertura(void) {
  static const char *const nuvio[] = {
    "pauseOverlayEnabled",
    "modernLandscapePostersEnabled",
    "modernHeroFullScreenBackdropEnabled",
    "collapseSidebar",
    "modernSidebar",
    "modernSidebarBlur",
    "heroSectionEnabled",
    "discoverLocation",
    "posterLabelsEnabled",
    "catalogAddonNameEnabled",
    "catalogTypeSuffixEnabled",
    "hideUnreleasedContent",
    "homeImdbRatingsVisibility",
    "classicFocusGradientEnabled",
    "continueWatchingEnabled",
    "continueWatchingCardStyle",
    "useEpisodeThumbnailsInCw",
    "blurContinueWatchingNextUp",
    "nextUpFromFurthestEpisode",
    "showUnairedNextUp",
    "continueWatchingSortMode",
    "blurUnwatchedEpisodes",
    "detailPageTrailerButtonEnabled",
    "preferExternalMetaAddonDetail",
    "showFullReleaseDate",
    "focusedPosterBackdropExpandEnabled",
    "focusedPosterBackdropExpandDelaySeconds",
    "fastHorizontalNavigationEnabled",
    "cardDepthEnabled",
    "cardDepthEdgeStrength",
    "cardDepthSheenStrength",
    "cardDepthEdgeCoverage",
    "cardDepthPostersEnabled",
    "cardDepthContinueWatchingEnabled",
    "cardDepthEpisodeCardsEnabled",
    "cardDepthCastEnabled",
    "cardDepthTrailersEnabled",
    "posterCardWidthDp",
    "posterCardCornerRadiusDp",
    "tmdb_enabled",
    "tmdb_language",
    "tmdb_use_artwork",
    "tmdb_use_basic_info",
    "tmdb_use_details",
    "tmdb_use_release_dates",
    "tmdb_use_credits",
    "tmdb_use_productions",
    "tmdb_use_networks",
    "tmdb_use_episodes",
    "tmdb_use_trailers",
    "tmdb_use_more_like_this",
    "tmdb_use_collections",
    "tmdb_enrich_continue_watching",
    "mdblist_enabled",
    "mdblist_show_trakt",
    "mdblist_show_imdb",
    "mdblist_show_tmdb",
    "mdblist_show_letterboxd",
    "mdblist_show_tomatoes",
    "mdblist_show_audience",
    "mdblist_show_metacritic",
    "mdblist_show_mal",
    "trailerCartazModo",
    "streamAutoPlaySource",
  };
  int cobertas=0,conta=0;
  for(int i=0;i<AJ_N;i++) {
    if(!CHAVE[i] || CHAVE[i][0]=='-' || OPCOES[i].tipo==OP_ACAO || OPCOES[i].tipo==OP_LEITURA)continue;
    if(i==AJ_PROVEDORES_ESCOPO || i==AJ_ENVIO_AUTO)continue; // destination consent/policy
    if(ajLegendaPrincipal(i) || ajFalhaBackend(i))continue; // real module adapters
    if(i==AJ_TEMA)continue; // fixed account themes / dynamic native exception tested below
    int cloud=0;
    for(size_t k=0;k<sizeof nuvio/sizeof *nuvio;k++)if(!strcmp(CHAVE[i],nuvio[k]))cloud=1;
    int scopes=0;
    for(int scope=AJ_BACKUP_APARELHO;scope<=AJ_BACKUP_HARDWARE;scope++)scopes+=abLocal(i,scope,valor[i]);
    if(scopes!=(cloud?0:1)) {
      fprintf(stderr,"Missing or overlapping backup classification: %s (%s)\n",CHAVE[i],OPCOES[i].rotulo);
      assert(0);
    }
    if(cloud){conta++;continue;}
    cobertas++;
    // Check actual capture/export/merge at both ends of every native choice's
    // supported range; the source Settings values must remain untouched.
    int low=OPCOES[i].tipo==OP_NUMERO?OPCOES[i].min:0;
    int high=OPCOES[i].tipo==OP_NUMERO?OPCOES[i].max:nValores(i)-1;
    for(int value=low;;value=high) {
      int scope=abLocal(i,AJ_BACKUP_HARDWARE,value)?AJ_BACKUP_HARDWARE:
        abLocal(i,AJ_BACKUP_PERFIL,value)?AJ_BACKUP_PERFIL:AJ_BACKUP_APARELHO;
      int old=valor[i];valor[i]=value;
      char line[128],*backup=NULL,*merged=NULL;
      snprintf(line,sizeof line,"%s %d\n",CHAVE[i],i==AJ_IDIOMA?ajustes_idioma():value);
      assert(ajustes_backup_capturar(scope,&backup) && strstr(backup,line));
      assert(ajustes_backup_mesclar(scope,"futureKey unchanged\n",backup,&merged));
      assert(strstr(merged,line) && strstr(merged,"futureKey unchanged\n"));
      assert(valor[i]==value);valor[i]=old;free(backup);free(merged);
      if(value==high)break;
    }
  }
  printf("PASS: %d native scalar choices round-trip; %d Nuvio choices stay separate\n",cobertas,conta);
}
