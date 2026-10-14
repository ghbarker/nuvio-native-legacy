// Separate translation unit avoids sharing player.c's private static names.
#include "posplay.c"
int probe_rewound(void) { return recuou; }
int probe_waiting(void) { return aguardaBusca; }
Uint32 probe_countdown(void) { return fecharEm; }
