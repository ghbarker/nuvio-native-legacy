package space.nuvio.nativelegacy

import org.junit.Assert.*
import org.junit.Test

class PlayerExternoContratoTest {
    private val url = "https://cdn.example/video?token=a%2Fb+z&sig=A%3D&x=1&x=2#part"
    private val mx = PlayerExternoContrato.alvosCabecalhos
    private fun fonte(cab: String = "", posicao: Long = 90001) =
        requireNotNull(PlayerExternoContrato.fonte(url, cab, "Episode", posicao))

    @Test fun ordinaryStreamsUseOneImplicitSystemChooserTarget() {
        val destinos = PlayerExternoContrato.destinos(fonte(), emptyList())
        assertEquals(1, destinos.size)
        val alvo = destinos.single()
        assertNull(alvo.alvo)
        assertEquals(url, alvo.url)
        assertEquals("video/*", alvo.mime)
        assertEquals("Episode", alvo.extras["title"])
        assertTrue(alvo.extras["position"] is Int)
        assertEquals(90001, alvo.extras["position"])
        assertEquals(false, alvo.extras["from_start"])
        assertFalse(alvo.extras.containsKey("headers"))
        assertFalse(alvo.extras.containsKey("subtitles_location"))
    }

    @Test fun requiredHeadersUseOnlyExplicitVerifiedMxComponents() {
        val cab = "User-Agent: Nuvio Test\r\nReferer: https://origin.example/a?x=1\nAuthorization: Bearer fixture\nCookie: a=b"
        val destinos = PlayerExternoContrato.destinos(fonte(cab), mx)
        assertEquals(mx, destinos.map { it.alvo })
        for (destino in destinos) {
            assertNotNull(destino.alvo)
            assertEquals(url, destino.url)
            assertEquals("video/*", destino.mime)
            assertEquals(true, destino.extras["return_result"])
            assertEquals(true, destino.extras["secure_uri"])
            assertArrayEquals(arrayOf("User-Agent", "Nuvio Test", "Referer", "https://origin.example/a?x=1",
                "Authorization", "Bearer fixture", "Cookie", "a=b"), destino.extras["headers"] as Array<*>)
        }
        // Each explicit target owns its array; changing one cannot strip or
        // retarget another player's required request headers.
        @Suppress("UNCHECKED_CAST")
        val primeira = destinos[0].extras["headers"] as Array<String>
        primeira[0] = "Changed"
        assertEquals("User-Agent", (destinos[1].extras["headers"] as Array<*>)[0])
    }

    @Test fun absentOrUnsupportedHeaderPlayersNeverCreateGenericBase() {
        val origem = fonte("Referer: https://origin.example/")
        assertTrue(PlayerExternoContrato.destinos(origem, emptyList()).isEmpty())
        val falsos = listOf(PlayerExternoContrato.Alvo("org.videolan.vlc", "org.videolan.vlc.gui.video.VideoPlayerActivity"),
            PlayerExternoContrato.Alvo(mx[0].pacote, "other.Activity"))
        assertTrue(PlayerExternoContrato.destinos(origem, falsos).isEmpty())
        assertEquals(listOf(mx[1]), PlayerExternoContrato.destinos(origem, falsos + mx[1]).map { it.alvo })
    }

    @Test fun malformedHeadersRefuseRatherThanSilentlyDropCredentials() {
        val invalidos = listOf("Authorization", ": missing", "Bad Name: value", "Origin: good\rbad",
            "Cookie: a\u0000b", "X-Test: a\tb", "Çustom: value")
        for (cab in invalidos) {
            val origem = fonte(cab)
            assertNull(origem.cabecalhos)
            assertTrue(PlayerExternoContrato.destinos(origem, mx).isEmpty())
        }
    }

    @Test fun urlsAndCapacityAreValidatedWithoutRewritingSignedQuery() {
        val invalidos = listOf("", "https://", "https:///path", "https://?q", "https://#fragment",
            "file:///private/video", "content://private/video", "intent://video", "ftp://cdn.example/video",
            " https://cdn.example/video", "https://cdn.example/a b", "https://cdn.example/a\nb")
        for (valor in invalidos) assertNull(PlayerExternoContrato.fonte(valor, "", "", 0))
        assertEquals(url, fonte().url)
        val prefixo = "https://cdn.example/?signature="
        val maximo = prefixo + "x".repeat(4095 - prefixo.length)
        assertEquals(maximo, PlayerExternoContrato.fonte(maximo, "X: " + "a".repeat(2044), "t".repeat(255), 0)?.url)
        assertNull(PlayerExternoContrato.fonte(maximo + "x", "", "", 0))
        assertNull(PlayerExternoContrato.fonte(url, "X: " + "a".repeat(2045), "", 0))
        assertNull(PlayerExternoContrato.fonte(url, "", "t".repeat(256), 0))
        assertNull(PlayerExternoContrato.fonte(url, "", "é".repeat(128), 0)) // byte limit
    }

    @Test fun resumeUsesIntMillisecondsWithoutOutputOnlyVlcExtras() {
        for (posicao in listOf(-1L, Long.MIN_VALUE, Int.MAX_VALUE.toLong() + 1, Long.MAX_VALUE))
            assertEquals(0, fonte(posicao = posicao).posicaoMs)
        for (posicao in listOf(0L, 1L, 90001L, Int.MAX_VALUE.toLong())) {
            val destino = PlayerExternoContrato.destinos(fonte(posicao = posicao), emptyList()).single()
            assertEquals(posicao.toInt(), destino.extras["position"])
            assertEquals(posicao == 0L, destino.extras["from_start"])
            assertFalse(destino.extras.containsKey("extra_position"))
            assertFalse(destino.extras.containsKey("extra_duration"))
        }
    }
}
