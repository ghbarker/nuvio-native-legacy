import space.nuvio.nativelegacy.RegistroDownload

fun longSecretPreflight() {
    val cases = listOf(
        "long plain quoted secret" to ("password=\"" + "A".repeat(RegistroDownload.MAX_BYTES - 32) + "\""),
        "long escaped quoted secret" to ("password=\"" + "\\\"".repeat(30000) + "\"")
    )
    var failures = 0
    for ((name, input) in cases) {
        try {
            val actual = RegistroDownload.limpar(input)
            check("AAAA" !in actual && "\\\"\\\"" !in actual) { "Secret content retained" }
            check(actual.length < 64) { "Redaction did not replace long quoted value" }
            println("PASS $name")
        } catch (e: StackOverflowError) {
            failures++
            println("FAIL $name: StackOverflowError in RegistroDownload.limpar")
        } catch (e: Throwable) {
            failures++
            println("FAIL $name: ${e.javaClass.simpleName}: ${e.message}")
        }
    }
    check(failures == 0) { "$failures long-secret cases failed" }
    println("REGISTRO_DOWNLOAD_LONG_SECRET_OK 2")
}

fun malformedSecretPreflight() {
    val cases = listOf(
        "unterminated double-quoted secret with spaces" to listOf(
            "password=\"double secret with spaces" to "[…]"
        ),
        "unterminated single-quoted secret with spaces" to listOf(
            "password='single secret with spaces" to "[…]"
        ),
        "trailing escape at EOF in quoted secrets" to listOf(
            "password=\"trailing secret \\" to "[…]",
            "password='trailing secret \\" to "[…]"
        ),
        "unterminated quoted secret stops before next diagnostic line" to listOf(
            "password=\"line secret words\nnext diagnostic preserved" to "[…]\nnext diagnostic preserved",
            "password='line secret words\nnext diagnostic preserved" to "[…]\nnext diagnostic preserved",
            "password=\"line secret words\r\nnext diagnostic preserved" to "[…]\r\nnext diagnostic preserved",
            "password='line secret words\rnext diagnostic preserved" to "[…]\rnext diagnostic preserved"
        )
    )
    var failures = 0
    for ((name, variants) in cases) {
        try {
            for ((input, expected) in variants) {
                val actual = RegistroDownload.limpar(input)
                check(actual == expected) { "Unterminated quoted value leaked or changed following diagnostic: $actual" }
            }
            println("PASS $name")
        } catch (e: Throwable) {
            failures++
            println("FAIL $name: ${e.javaClass.simpleName}: ${e.message}")
        }
    }
    check(failures == 0) { "$failures malformed-secret cases failed" }
    println("REGISTRO_DOWNLOAD_MALFORMED_SECRET_OK 4")
}

fun main() {
    longSecretPreflight()
    malformedSecretPreflight()
}
