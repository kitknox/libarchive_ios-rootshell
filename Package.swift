// swift-tools-version: 5.9
import PackageDescription

let package = Package(
    name: "libarchive_ios-rootshell",
    platforms: [
        .iOS(.v14),
        .macCatalyst(.v14),
        .visionOS(.v1),
    ],
    products: [
        .library(name: "libarchive_ios", targets: ["libarchive_ios"]),
    ],
    targets: [
        .binaryTarget(
            name: "libarchive_ios",
            url: "https://github.com/kitknox/libarchive_ios-rootshell/releases/download/v0.1.2/libarchive_ios.xcframework.zip",
            checksum: "e9d3dd0dd377509d1424471ef7a5b0c9dd1c7013517a4996a5e88f4e41de51fd"
        ),
    ]
)
