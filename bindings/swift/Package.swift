// swift-tools-version: 5.9

import PackageDescription

let package = Package(
    name: "HesiaCore",
    products: [
        .library(name: "HesiaCore", targets: ["HesiaCore"])
    ],
    targets: [
        .target(
            name: "CHesiaCore",
            publicHeadersPath: "include",
            cSettings: [
                .headerSearchPath("../../../include")
            ],
            linkerSettings: [
                .linkedLibrary("hesia_core")
            ]
        ),
        .target(
            name: "HesiaCore",
            dependencies: ["CHesiaCore"]
        )
    ]
)
