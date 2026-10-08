package space.nuvio.nativelegacy

fun main() {
    val s = PuloAutomatico()
    s.abrir(10, 1, 0)
    check(s.pausar(10, 1, 1, true)); check(!s.tocar)
    check(s.pausar(10, 1, 2, false)); check(s.tocar)
    s.abrir(10, 2, 100) // Retry preserva o controle nativo.
    check(s.controle == 2 && s.busca == 0 && s.estado == 0)
    check(s.observar(10, 2, 200, 1000, true, false))
    check(!s.observar(10, 1, 201, 120000, true, true)) // Ignorar a instancia anterior.
    check(!s.pausar(9, 2, 3)); check(!s.pausar(10, 1, 3))
    check(s.buscar(10, 2, 3, 90000, 300))
    s.descontinuidade(10, 2, 90000)
    s.observar(10, 2, 400, 90000, true, false); check(s.estado == 0)
    s.observar(10, 2, 650, 90250, true, false); check(s.estado == 1)
    s.abrir(10, 3, 700) // Retry descarta a evidencia do seek confirmado.
    check(s.controle == 3 && s.busca == 3 && s.estado == 0)
    s.preparado(10, 3, 90000, true); check(s.estado == 0)
    s.observar(10, 3, 800, 90000, false, false); check(s.estado == 0) // sem first-frame/isPlaying
    s.observar(10, 3, 1000, 90000, true, false); check(s.estado == 0)
    s.observar(10, 3, 1250, 90250, true, false); check(s.estado == 0) // seek ja resolvido; nao fabrica ACK novo
    check(s.buscar(10, 3, 4, 119000, 1300))
    check(s.pausar(10, 3, 5)); check(s.pausar(10, 3, 6)); check(s.estado == 0) // nao resolve seek
    s.abrir(10, 4, 1400) // Retry preserva os seriais do seek pendente.
    check(s.controle == 6 && s.busca == 4 && s.estado == 0)
    s.descontinuidade(10, 3, 119000) // Ignorar a descontinuidade da instancia anterior.
    s.preparado(10, 3, 119000, true) // Ignorar a preparacao da instancia anterior.
    s.observar(10, 4, 1500, 119000, true, false)
    s.observar(10, 4, 1750, 119250, true, false); check(s.estado == 0)
    s.preparado(10, 4, 118000, true) // abriu antes do alvo: sem prova de seek
    s.observar(10, 4, 1800, 119500, true, false); check(s.estado == 0)
    s.preparado(10, 4, 119000, false) // inicializacao recusada
    s.observar(10, 4, 1900, 119750, true, false); check(s.estado == 0)
    s.preparado(10, 4, 119000, true)
    s.observar(10, 4, 2000, 119000, true, false); check(s.estado == 0)
    s.observar(10, 4, 2250, 119250, true, false); check(s.estado == 1)
    check(s.buscar(10, 4, 7, 120000, 2300))
    s.abrir(10, 5, 2400); s.observar(10, 5, 2500, 120000, false, true)
    check(s.estado == 1 && s.controle == 7) // fim real do novo player recupera
    s.abrir(11, 6, 2600)
    check(s.controle == 0 && s.busca == 0 && s.estado == 0)
    check(!s.observar(10, 5, 2601, 120000, true, true))
    check(s.pausar(11, 6, 1, true));check(!s.tocar)
    s.abrir(11, 7, 2700, s.tocar) // retry da sessao pausada nao a despausa
    check(!s.tocar && s.controle == 1)
    check(!s.pausar(11, 6, 2, false));check(!s.tocar) // callback velho nao despausa
    s.abrir(12, 8, 2800);check(s.tocar && s.controle == 0)
    val confirmado = PuloAutomatico()
    confirmado.abrir(20, 1, 0); confirmado.buscar(20, 1, 1, 90000, 1)
    confirmado.descontinuidade(20, 1, 90000)
    confirmado.observar(20, 1, 100, 90000, true, false)
    confirmado.observar(20, 1, 350, 90250, true, false);check(confirmado.estado == 1)
    confirmado.abrir(20, 2, 400)
    confirmado.preparado(20, 2, 90250, true) // Retry retoma depois do alvo.
    check(confirmado.observar(20, 2, 500, 90500, true, false))
    check(confirmado.controle == 1 && confirmado.busca == 1 && confirmado.estado == 0)
    confirmado.observar(20, 2, 20000, 110000, true, false)
    check(confirmado.estado == 0) // nao converte seek antigo resolvido em timeout
    println("PuloAutomatico: retry/control/seek freshness, preparation, stale instance, end passed")
}
