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
            url: "https://github.com/kitknox/libarchive_ios-rootshell/releases/download/v0.1.0/libarchive_ios.xcframework.zip",
            checksum: "33a2bf775e0c36fd9f184d34241db4092ceabf7e1301f80b04be17d0c80a995a"
        ),
    ]
)
