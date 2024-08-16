// swift-tools-version: 5.9
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "OggAudioConverter",
    platforms: [
        .iOS(.v13), .macOS(.v10_14)
    ],
    products: [
        .library(
            name: "OggAudioConverter",
            targets: ["OggAudioConverter"]
        ),
    ],
    dependencies: [],
    targets: [
        .target(
            name: "OggAudioConverter",
            dependencies: [
                .target(name: "ogg"),
                .target(name: "vorbis")
            ],
            path: "Sources/OggAudioConverter",
            publicHeadersPath: "include",
            cSettings: [
                .headerSearchPath("Sources/OggAudioConverter")
            ]
        ),
        .binaryTarget(
            name: "ogg",
            path: "Frameworks/ogg.xcframework"
        ),
        .binaryTarget(
            name: "vorbis",
            path: "Frameworks/vorbis.xcframework"
        ),
        .testTarget(
            name: "OggAudioConverterTests",
            dependencies: ["OggAudioConverter"],
            path: "Tests"
        ),
    ]
)
