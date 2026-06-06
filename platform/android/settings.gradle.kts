// Standalone Gradle project for the dice3d Android library (produces an AAR).
// Mirrors RPGPT's toolchain (AGP 8.7.3 / Kotlin 2.1.0 / Gradle 8.11.1) so the
// AAR is consumable by the app, and so RPGPT can also `includeBuild` this dir.
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
