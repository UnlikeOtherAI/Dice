// swift-tools-version:5.9
import PackageDescription

let package = Package(
    name: "Dice3D",
    platforms: [.iOS(.v14)],
    products: [
        .library(name: "Dice3D", targets: ["Dice3D"]),
    ],
    targets: [
        .target(
            name: "Dice3D",
            path: "Sources/Dice3D"
        ),
    ]
)
