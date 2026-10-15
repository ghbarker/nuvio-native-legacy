package android.net

data class Uri(val value: String) {
    companion object { fun parse(value: String) = Uri(value) }
}
