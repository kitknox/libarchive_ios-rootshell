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
            url: "https://github.com/kitknox/libarchive_ios-rootshell/releases/download/v0.1.1/libarchive_ios.xcframework.zip",
            checksum: "30c21b6f7091fcd1eb9eb590e14097e65781de61f82f19f67771854f61c95dc5"
        ),
    ]
)
