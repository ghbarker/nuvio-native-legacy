plugins {
    id("com.android.application") version "8.7.3"
    id("org.jetbrains.kotlin.android") version "2.0.21"
}
android {
    namespace = "space.nuvio.livebench"
    compileSdk = 35
    defaultConfig {
        applicationId = "space.nuvio.livebench"
        minSdk = 26
        targetSdk = 35
        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
    }
    sourceSets["main"].java.srcDir(layout.buildDirectory.dir("production-src"))
    sourceSets["test"].java.srcDir(layout.buildDirectory.dir("production-tests"))
    compileOptions { sourceCompatibility = JavaVersion.VERSION_17; targetCompatibility = JavaVersion.VERSION_17 }
    kotlinOptions { jvmTarget = "17" }
    testOptions { unitTests.isReturnDefaultValues = true }
}
dependencies {
    implementation("androidx.media3:media3-exoplayer:1.8.0")
    implementation("androidx.media3:media3-exoplayer-hls:1.8.0")
    implementation("androidx.core:core-ktx:1.13.1")
    testImplementation("junit:junit:4.13.2")
    androidTestImplementation("androidx.test:runner:1.6.2")
    androidTestImplementation("androidx.test:rules:1.6.1")
    androidTestImplementation("androidx.test.ext:junit:1.2.1")
}

// Select unchanged production files at build time; the harness has no native app.
val copyProduction = tasks.register<Sync>("copyProduction") {
    from("../../android/app/src/main/java") {
        include("space/nuvio/nativelegacy/LiveLoadRecovery.kt", "space/nuvio/nativelegacy/LiveLoadErrorPolicy.kt",
            "space/nuvio/nativelegacy/LiveWindowRecovery.kt", "space/nuvio/nativelegacy/LivePlaylistDataSource.kt",
            "space/nuvio/nativelegacy/ParaleloDataSource.kt", "space/nuvio/nativelegacy/PassivoMedidor.kt")
    }
    into(layout.buildDirectory.dir("production-src"))
}
val copyTests = tasks.register<Sync>("copyProductionTests") {
    from("../../android/app/src/test/java") {
        include("android/net/Uri.java", "space/nuvio/nativelegacy/LiveLoadRecoveryTest.kt",
            "space/nuvio/nativelegacy/LiveLoadErrorPolicyTest.kt", "space/nuvio/nativelegacy/LiveWindowRecoveryTest.kt",
            "space/nuvio/nativelegacy/LivePlaylistDataSourceTest.kt")
    }
    into(layout.buildDirectory.dir("production-tests"))
}
tasks.named("preBuild") { dependsOn(copyProduction, copyTests) }
