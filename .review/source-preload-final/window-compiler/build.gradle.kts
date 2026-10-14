repositories { mavenCentral() }
val windowCompiler by configurations.creating
dependencies {
    windowCompiler("org.jetbrains.kotlin:kotlin-compiler-embeddable:2.0.21")
    windowCompiler("org.jetbrains.kotlin:kotlin-stdlib:2.0.21")
    windowCompiler("org.jetbrains.kotlin:kotlin-script-runtime:2.0.21")
    windowCompiler("org.jetbrains.intellij.deps:trove4j:1.0.20200330")
    windowCompiler("org.jetbrains.kotlinx:kotlinx-coroutines-core-jvm:1.6.4")
    windowCompiler("org.jetbrains:annotations:13.0")
}
tasks.register("resolveWindowCompiler") {
    doLast {
        windowCompiler.resolve().sortedBy { it.name }.forEach { println(it.name) }
    }
}
