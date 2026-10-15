#!/usr/bin/env python3
"""Keep original flags and source lists; add only omitted fixture dependencies."""
import os,sys,subprocess,json
from pathlib import Path
args=sys.argv[1:];root=Path(os.environ['PROFILE_LEGACY_COMPANIONS']);extra=[];label=None
if 'tests/jellyfin.c' in args:
 args[args.index('tests/jellyfin.c')]=str(root/'tests/jellyfin.c')
 label='original HTTP assertions with initialized secret buffer'
if 'tests/player_android_retomada.c' in args:
 extra.append(str(root/'android-player-preload-off.c'))
if 'tests/perfilsel.c' in args:
 args[args.index('tests/perfilsel.c')]=str(root/'tests/perfilsel.c')
 extra.append(str(root/'perfilsel-session.c'))
 label='original selector assertions with captured-identity RPC transport'
names=['fontecache_vod','fonte_ordem_addon','stream_faixas_ui','addons_proveniencia','fontefalha_worker','fontefalha_playback','addons_legendas','fontesmortas_orcamento']
if '-c' not in args:
 for name in names:
  source='tests/'+name+'.c'
  if source not in args:continue
  extra.append(str(root/'epoch.c'))
  if name in ['addons_proveniencia','fontesmortas_orcamento']:
   args[args.index(source)]=str(root/source);label='legacy assertions + dispatch snapshot setup'
  if name=='fontefalha_playback':extra.append(str(root/'playback-neighbours.c'))
  if name in ['fontefalha_playback','fontesmortas_orcamento']:extra.append(str(root/'preload-off-neighbours.c'))
  if name=='fontesmortas_orcamento':extra.append(str(root/'budget-neighbours.c'))
  break
# This wrapper is only installed for the explicitly listed legacy gate, not full
# source links or the real-profile ownership tests. The outer compiler logger
# and SDL3 rpath wrapper remain authoritative over original/effective arguments.
real=os.environ['PROFILE_LEGACY_REAL_CC'];result=subprocess.run([real,*args,*extra]);sys.exit(result.returncode)
