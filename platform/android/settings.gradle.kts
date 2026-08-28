// Standalone Gradle project for the dice3d Android library (produces an AAR).
// Pinned to a common Android toolchain (AGP 8.7.3 / Kotlin 2.1.0 / Gradle
// 8.11.1) so the AAR is consumable by a host app, which can also `includeBuild`
// this directory instead of consuming the built artifact.
pluginManagement {
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
    }
}

dependencyResolutionManagement {
    repositories {
        google()
        mavenCentral()
    }
}

rootProject.name = "dice3d"
