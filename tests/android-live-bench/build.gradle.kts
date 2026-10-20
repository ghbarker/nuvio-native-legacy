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
    // Compile the actual production policy; no copied implementation or native APK.
    sourceSets["main"].java.apply {
        srcDir("../../android/app/src/main/java")
        include("space/nuvio/livebench/**", "space/nuvio/nativelegacy/LiveLoadRecovery.kt",
            "space/nuvio/nativelegacy/LiveLoadErrorPolicy.kt", "space/nuvio/nativelegacy/LiveWindowRecovery.kt",
            "space/nuvio/nativelegacy/LivePlaylistDataSource.kt", "space/nuvio/nativelegacy/ParaleloDataSource.kt",
            "space/nuvio/nativelegacy/PassivoMedidor.kt")
    }
    sourceSets["test"].java.apply {
        srcDir("../../android/app/src/test/java")
        include("space/nuvio/nativelegacy/LiveLoadRecoveryTest.kt")
    }
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
