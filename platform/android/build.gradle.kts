// dice3d Android library — packages the Kotlin wrapper (DiceView / DiceRenderer)
// + the prebuilt libdice3d.so (built by scripts/build_android.sh) into an AAR.
//   ./gradlew assembleRelease   → build/outputs/aar/dice3d-release.aar
plugins {
    id("com.android.library") version "8.7.3"
    id("org.jetbrains.kotlin.android") version "2.1.0"
}

group = "com.dice3d"
version = "0.2.1"

android {
    namespace = "com.dice3d"
    compileSdk = 35

    defaultConfig {
        minSdk = 26
        ndk { abiFilters += listOf("arm64-v8a") }
    }

    sourceSets["main"].apply {
        manifest.srcFile("src/main/AndroidManifest.xml")
        java.srcDirs("src/main/java")
        jniLibs.srcDirs("src/main/jniLibs")
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions {
        jvmTarget = "17"
    }
    // .so is prebuilt for arm64-v8a; keep it as-is in the AAR.
    packaging {
        jniLibs.keepDebugSymbols += "**/*.so"
    }
}

// No external deps: the wrapper uses only android.view.* + System.loadLibrary.

