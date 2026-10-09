package space.nuvio.nativelegacy

// Geometry only: the Activity applies these bounds to the shared SDL/video parent.
internal object TouchViewport {
    data class Bounds(val left: Int, val top: Int, val width: Int, val height: Int)

    fun fit(width: Int, height: Int, left: Int, top: Int, right: Int, bottom: Int): Bounds? {
        val safeWidth = width - left - right
        val safeHeight = height - top - bottom
        if (safeWidth <= 0 || safeHeight <= 0) return null
        val contentWidth: Int
        val contentHeight: Int
        if (safeWidth.toLong() * 9 <= safeHeight.toLong() * 16) {
            contentWidth = safeWidth
            contentHeight = (safeWidth.toLong() * 9 / 16).toInt().coerceAtLeast(1)
        } else {
            contentHeight = safeHeight
            contentWidth = (safeHeight.toLong() * 16 / 9).toInt().coerceAtLeast(1)
        }
        return Bounds(left + (safeWidth - contentWidth) / 2, top + (safeHeight - contentHeight) / 2,
            contentWidth, contentHeight)
    }
}
