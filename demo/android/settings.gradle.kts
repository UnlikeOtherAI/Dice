pluginManagement {
    repositories { google(); mavenCentral(); gradlePluginPortal() }
}
dependencyResolutionManagement {
    repositories { google(); mavenCentral() }
}
rootProject.name = "dice3d-demo"
include(":app")
// Consume the dice3d library from this repo as a composite build.
includeBuild("../../platform/android")
