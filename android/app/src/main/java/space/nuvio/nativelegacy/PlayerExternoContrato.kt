package space.nuvio.nativelegacy

import java.net.URI
import java.util.Locale

/** Payload policy only: no Android calls, network access or playback tracking. */
internal object PlayerExternoContrato {
    data class Alvo(val pacote: String, val classe: String)
    data class Fonte(val url: String, val titulo: String, val posicaoMs: Int,
                     val cabecalhos: List<Pair<String, String>>?)
    data class Destino(val alvo: Alvo?, val url: String, val mime: String,
                       val extras: Map<String, Any>)

    // MX's published contract uses alternating name/value strings, not lines.
    // VLC's current external Intent route has no arbitrary-header consumer.
    val alvosCabecalhos = listOf(
        Alvo("com.mxtech.videoplayer.ad", "com.mxtech.videoplayer.ad.ActivityScreen"),
        Alvo("com.mxtech.videoplayer.pro", "com.mxtech.videoplayer.ActivityScreen")
    )

    fun fonte(url: String, cab: String, titulo: String, posicaoMs: Long): Fonte? {
        if (url.toByteArray(Charsets.UTF_8).size >= 4096 || cab.toByteArray(Charsets.UTF_8).size >= 2048 ||
            titulo.toByteArray(Charsets.UTF_8).size >= 256) return null
        if (url.any { it.code <= 32 || it.code == 127 }) return null
        val uri = try { URI(url) } catch (_: Exception) { return null }
        if (uri.scheme?.lowercase(Locale.ROOT) !in setOf("http", "https") || uri.host.isNullOrEmpty()) return null
        val posicao = if (posicaoMs in 0L..Int.MAX_VALUE.toLong()) posicaoMs.toInt() else 0
        return Fonte(url, titulo, posicao, cabecalhos(cab))
    }

    private fun cabecalhos(cab: String): List<Pair<String, String>>? {
        val saida = ArrayList<Pair<String, String>>()
        for (linha in cab.split('\n')) {
            val texto = linha.removeSuffix("\r")
            if (texto.any { (it.code < 32 && it != '\t') || it.code == 127 }) return null
            if (texto.isBlank()) continue
            val separador = texto.indexOf(':')
            if (separador <= 0) return null
            val nome = texto.substring(0, separador).trim()
            val valor = texto.substring(separador + 1).trim()
            if (nome.isEmpty() || nome.any { !it.isLetterOrDigit() && it !in "!#$%&'*+-.^_`|~" } ||
                nome.any { it.code > 127 } || valor.any { it.code < 32 || it.code == 127 }) return null
            saida.add(nome to valor)
        }
        return saida.toList()
    }

    // Empty means refusal, never a generic headerless fallback. Every required
    // header is attached only to an explicit, contract-supported component.
    fun destinos(fonte: Fonte, disponiveis: Collection<Alvo>): List<Destino> {
        val cab = fonte.cabecalhos ?: return emptyList()
        val extras = linkedMapOf<String, Any>(
            "title" to fonte.titulo, "position" to fonte.posicaoMs,
            "from_start" to (fonte.posicaoMs == 0), "return_result" to true
        )
        if (cab.isEmpty()) return listOf(Destino(null, fonte.url, "video/*", extras))
        val headers = cab.flatMap { listOf(it.first, it.second) }.toTypedArray()
        return alvosCabecalhos.filter { it in disponiveis }.map {
            Destino(it, fonte.url, "video/*", extras + mapOf("headers" to headers.copyOf(), "secure_uri" to true))
        }
    }
}
