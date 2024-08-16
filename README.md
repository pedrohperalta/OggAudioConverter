
# OggAudioConverter

**OggAudioConverter** is a Swift package that provides a function to convert Ogg audio files to other formats, such as WAV. The package is built using a combination of Swift and Objective-C, and it leverages the `ogg.xcframework` and `vorbis.xcframework` libraries.

## Features

- Convert Ogg files to WAV format.
- Support for iOS and macOS platforms.
- Built-in handling of Ogg and Vorbis codecs through included frameworks.

## Requirements

- iOS 13.0+ / macOS 10.14+
- Swift 5.9+
- Xcode 14.2+

## Installation

### Swift Package Manager

You can add `OggAudioConverter` to your project using [Swift Package Manager](https://swift.org/package-manager/).

1. In Xcode, select **File > Add Packages**.
2. Enter the package URL:

   ```
   https://github.com/pedrohperalta/OggAudioConverter.git
   ```

3. Choose the version or branch you want to install.

Alternatively, you can add it directly to your `Package.swift` file:

```swift
dependencies: [
    .package(url: "https://github.com/yourusername/OggAudioConverter.git", from: "0.1.0")
]
```

## Usage

First, import the `OggAudioConverter` module:

```swift
import OggAudioConverter
```

Then, you can use the `OggConverter` class to convert Ogg files to WAV:

```swift
let converter = OggConverter()
converter.convertOggToWav("input.ogg", outputFilePath: "output.wav")
```

## Credits

This package uses the following frameworks:

- **[ogg.xcframework](https://github.com/xiph/ogg)**: A library for working with Ogg bitstreams.
- **[vorbis.xcframework](https://github.com/xiph/vorbis)**: A library for working with Vorbis audio codecs.
