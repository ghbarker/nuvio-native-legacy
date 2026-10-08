package space.nuvio.nativelegacy

// Reabrir o decoder preserva os seriais nativos, mas descarta a evidencia antiga.
internal class PuloAutomatico {
    var controle = 0; private set
    var busca = 0; private set
    var estado = 0; private set
    var tocar = true; private set
    private var geracao = 0
    private var instancia = 0
    private var alvo = 0L
    private var pedidoEm = 0L
    private var pendente = false
    private var descontinuidade = false
    private var preparacao = false
    private var amostra = -1L
    private var amostraEm = 0L

    fun abrir(geracao: Int, instancia: Int, agora: Long, tocar: Boolean = true) {
        if (geracao != this.geracao) { controle = 0; busca = 0; alvo = 0L; pendente = false }
        this.tocar = tocar
        this.geracao = geracao; this.instancia = instancia; pedidoEm = agora
        suspender()
    }
    fun atual(geracao: Int, instancia: Int) = geracao == this.geracao && instancia == this.instancia
    fun suspender() {
        estado = 0; descontinuidade = false; preparacao = false; amostra = -1L
    }
    fun pausar(geracao: Int, instancia: Int, serial: Int, pausa: Boolean = false): Boolean {
        if (!atual(geracao, instancia) || serial <= controle) return false
        controle = serial
        tocar = !pausa
        return true // Pausa nao confirma um seek pendente.
    }
    fun buscar(geracao: Int, instancia: Int, serial: Int, alvo: Long, agora: Long): Boolean {
        if (!atual(geracao, instancia) || serial <= busca) return false
        controle = maxOf(controle, serial); busca = serial; this.alvo = alvo; pedidoEm = agora
        pendente = true
        suspender()
        return true
    }
    fun recusar() { estado = -1 }
    fun preparado(geracao: Int, instancia: Int, inicio: Long, aceito: Boolean) {
        if (!atual(geracao, instancia)) return
        preparacao = pendente && aceito && inicio == alvo
    }
    fun descontinuidade(geracao: Int, instancia: Int, pos: Long) {
        if (!atual(geracao, instancia) || busca == 0 || pos != alvo) return
        descontinuidade = true; amostra = -1L
    }
    fun observar(geracao: Int, instancia: Int, agora: Long, pos: Long, tocando: Boolean, fim: Boolean): Boolean {
        if (!atual(geracao, instancia)) return false
        if (pendente && estado <= 0) {
            if (fim) estado = 1
            else if ((descontinuidade || preparacao) && tocando && pos >= alvo) {
                if (amostra < 0L) { amostra = pos; amostraEm = agora }
                else if (agora > amostraEm && pos > amostra &&
                    pos - amostra <= (agora - amostraEm) * 4 + 100) estado = 1
            } else amostra = -1L
            if (estado == 0 && agora - pedidoEm > 10000L) estado = -1
            if (estado == 1) pendente = false
        }
        return true
    }
}
