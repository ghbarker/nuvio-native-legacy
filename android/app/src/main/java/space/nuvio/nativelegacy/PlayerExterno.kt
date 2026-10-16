package space.nuvio.nativelegacy

import android.app.AlertDialog
import android.content.ComponentName
import android.content.Intent
import android.net.Uri
import java.util.concurrent.ConcurrentLinkedQueue

/** A token owns the dialog, release barrier, launch and eventual return. */
internal class PlayerExterno(private val activity: NuvioActivity) {
    companion object { const val PEDIDO = 49119 }
    private enum class Etapa { ESCOLHA, PRONTO, LIBERANDO, ABERTO }
    private class Pedido(val token: Long, val fonte: PlayerExternoContrato.Fonte, val rotulos: List<String>,
                         var etapa: Etapa) {
        var dialogo: AlertDialog? = null
        var saiuDoHost = false
        var desvinculado = false
    }
    private val trava = Any()
    private var pedido: Pedido? = null
    private var retomado = false
    private var fechado = false
    // Independent of the IME/voice queue: those readers discard unknown events.
    private val eventos = ConcurrentLinkedQueue<LongArray>()
    val emAndamento: Boolean get() = synchronized(trava) { pedido != null }

    fun pedir(token: Long, modo: Int, url: ByteArray, cab: ByteArray, titulo: ByteArray,
              posicaoMs: Long, rotulos: Array<String>): Boolean {
        if (token <= 0 || modo !in 1..2 || rotulos.size != 7 ||
            url.size >= 4096 || cab.size >= 2048 || titulo.size >= 256) return false
        // Decode now; a queued UI runnable must not read mutable caller arrays.
        val urlTexto = String(url, Charsets.UTF_8)
        val cabTexto = String(cab, Charsets.UTF_8)
        if (!urlTexto.toByteArray(Charsets.UTF_8).contentEquals(url) ||
            !cabTexto.toByteArray(Charsets.UTF_8).contentEquals(cab)) return false
        val fonte = PlayerExternoContrato.fonte(urlTexto, cabTexto,
            String(titulo, Charsets.UTF_8), posicaoMs) ?: return false
        // Android validates its own URI as well; the original signed string is kept.
        val uri = try { Uri.parse(fonte.url) } catch (_: Exception) { return false }
        if (!(uri.scheme.equals("http", true) || uri.scheme.equals("https", true)) || uri.host.isNullOrEmpty()) return false
        val novo = Pedido(token, fonte, rotulos.toList(), if (modo == 2) Etapa.ESCOLHA else Etapa.PRONTO)
        synchronized(trava) {
            if (fechado || pedido != null) return false
            pedido = novo
        }
        activity.runOnUiThread {
            if (!atual(novo)) return@runOnUiThread
            if (activity.isFinishing || activity.isDestroyed) terminar(novo, 6)
            else if (modo == 2) perguntar(novo) else abrirMain(novo)
        }
        return true // accepted; launched is a separate, owned event
    }

    fun abrir(token: Long): Boolean {
        val dono = synchronized(trava) { pedido?.takeIf { it.token == token } } ?: return false
        activity.runOnUiThread { if (atual(dono)) abrirMain(dono) }
        return true
    }

    fun cancelar(token: Long) {
        val velho = synchronized(trava) {
            val p = pedido?.takeIf { it.token == token } ?: return
            // Native title/profile ownership can end before the external app
            // returns. Detach its result, but keep admission/icon protection.
            if (p.etapa == Etapa.ABERTO) { p.desvinculado = true; return }
            pedido = null
            p
        }
        NvPlayer.externoTerminar(token)
        activity.runOnUiThread { velho.dialogo?.dismiss(); velho.dialogo = null }
        // Never finish or otherwise control the other application's activity.
    }

    fun evento(): LongArray? = eventos.poll()
    private fun atual(p: Pedido) = synchronized(trava) { !fechado && pedido === p }

    private fun terminar(p: Pedido, evento: Int) {
        synchronized(trava) {
            if (pedido !== p) return
            pedido = null
        }
        p.dialogo?.dismiss(); p.dialogo = null
        NvPlayer.externoTerminar(p.token)
        if (!p.desvinculado) eventos.add(longArrayOf(p.token, evento.toLong()))
    }

    private fun perguntar(p: Pedido) {
        if (!atual(p) || p.etapa != Etapa.ESCOLHA) return
        p.dialogo = AlertDialog.Builder(activity).setTitle(p.rotulos[0])
            .setItems(arrayOf(p.rotulos[1], p.rotulos[2], p.rotulos[3])) { _, escolha ->
                if (atual(p) && p.etapa == Etapa.ESCOLHA) {
                    p.dialogo = null
                    when (escolha) {
                        0 -> terminar(p, 1)
                        1 -> {
                            p.etapa = Etapa.PRONTO
                            eventos.add(longArrayOf(p.token, 2))
                            // Native cancels its own workers before calling abrir.
                        }
                        else -> terminar(p, 5)
                    }
                }
            }.setOnCancelListener { if (atual(p)) terminar(p, 5) }.create()
        try { p.dialogo?.show() } catch (_: Exception) { terminar(p, 6) }
    }

    private fun oferecerInterno(p: Pedido, mensagem: String) {
        if (!atual(p)) return
        if (activity.isFinishing || activity.isDestroyed) { terminar(p, 6); return }
        p.dialogo?.dismiss()
        p.dialogo = AlertDialog.Builder(activity).setTitle(p.rotulos[0]).setMessage(mensagem)
            .setPositiveButton(p.rotulos[1]) { _, _ -> if (atual(p)) terminar(p, 1) }
            .setNegativeButton(p.rotulos[3]) { _, _ -> if (atual(p)) terminar(p, 5) }
            .setOnCancelListener { if (atual(p)) terminar(p, 5) }.create()
        try { p.dialogo?.show() } catch (_: Exception) { terminar(p, 6) }
    }

    private fun intent(destino: PlayerExternoContrato.Destino): Intent =
        Intent(Intent.ACTION_VIEW).setDataAndType(Uri.parse(destino.url), destino.mime).apply {
            destino.alvo?.let { component = ComponentName(it.pacote, it.classe) }
            for ((nome, valor) in destino.extras) when (valor) {
                is String -> putExtra(nome, valor)
                is Int -> putExtra(nome, valor)
                is Boolean -> putExtra(nome, valor)
                is Array<*> -> putExtra(nome, valor.map { it as String }.toTypedArray())
            }
        }

    private fun abrirMain(p: Pedido) {
        if (!atual(p) || p.etapa != Etapa.PRONTO) return
        p.etapa = Etapa.LIBERANDO // repeated native calls cannot open another chooser
        val disponiveis = PlayerExternoContrato.alvosCabecalhos.filter { alvo ->
            val candidato = Intent(Intent.ACTION_VIEW).setDataAndType(Uri.parse(p.fonte.url), "video/*")
                .setComponent(ComponentName(alvo.pacote, alvo.classe))
            try {
                val info = activity.packageManager.resolveActivity(candidato, 0)?.activityInfo
                info != null && info.enabled && info.exported && info.applicationInfo.enabled &&
                    info.packageName == alvo.pacote && info.name == alvo.classe
            } catch (_: Exception) { false }
        }
        val destinos = PlayerExternoContrato.destinos(p.fonte, disponiveis)
        if (destinos.isEmpty()) { oferecerInterno(p, p.rotulos[5]); return }
        val principais = destinos.map(::intent)
        val genericoDisponivel = principais.first().component != null || try {
            activity.packageManager.resolveActivity(principais.first(), 0) != null
        } catch (_: Exception) { false }
        if (!genericoDisponivel) {
            oferecerInterno(p, p.rotulos[4]); return
        }
        // The base is explicit for required-header streams. Initial intents are
        // also explicit: no headerless implicit Intent offers unsupported apps.
        val chooser = Intent.createChooser(principais.first(), p.rotulos[0]).apply {
            if (principais.size > 1) putExtra(Intent.EXTRA_INITIAL_INTENTS, principais.drop(1).toTypedArray())
        }
        val aceitou = synchronized(trava) {
            if (fechado || pedido !== p) return
            NvPlayer.externoLiberar(p.token) { liberou ->
                if (atual(p)) {
                    if (!liberou) oferecerInterno(p, p.rotulos[6])
                    else {
                        // Linearize launch with native-thread cancellation. A
                        // canceled owner cannot reacquire the barrier or launch
                        // after its cancellation has already returned.
                        synchronized(trava) {
                            if (fechado || pedido !== p) return@externoLiberar
                            p.etapa = Etapa.ABERTO
                            try {
                                @Suppress("DEPRECATION")
                                activity.startActivityForResult(chooser, PEDIDO)
                                eventos.add(longArrayOf(p.token, 3))
                            } catch (_: Exception) {
                                p.etapa = Etapa.LIBERANDO; p.saiuDoHost = false
                                oferecerInterno(p, p.rotulos[6])
                            }
                        }
                    }
                }
            }
        }
        if (!aceitou) oferecerInterno(p, p.rotulos[6])
    }

    fun pausou() {
        retomado = false
        synchronized(trava) { pedido?.takeIf { it.etapa == Etapa.ABERTO }?.saiuDoHost = true }
    }
    fun retomou() { retomado = true; verificarRetorno() }
    fun foco(volta: Boolean) { if (volta) verificarRetorno() }
    private fun verificarRetorno() {
        val p = synchronized(trava) { pedido } ?: return
        if (retomado && activity.hasWindowFocus() && p.etapa == Etapa.ABERTO && p.saiuDoHost)
            terminar(p, 4)
    }
    fun resultado(codigo: Int): Boolean {
        // Some singleTask players return CANCELED immediately at launch. Neither
        // result code nor extras prove playback; wait for an actual host return.
        return codigo == PEDIDO
    }
    fun fechar() {
        // The whole host is being destroyed; it cannot observe another return.
        val p = synchronized(trava) { fechado = true; val antigo = pedido; pedido = null; antigo }
        if (p != null) { p.dialogo?.dismiss(); NvPlayer.externoTerminar(p.token) }
        eventos.clear()
    }
}
