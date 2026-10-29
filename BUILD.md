# Building Descent 3 Open Source

## Dependencies

Builds require a C++17 compiler. Commercial game assets are not needed to
compile or install the engine: [first-run setup](USAGE.md#first-run-asset-setup)
asks for their location when you launch the game.

The build process uses [**CMake**](https://cmake.org/) and, by default, [**Ninja**](https://ninja-build.org/). You must install these; the project cannot locate them for you. The source code also depends on third-party libraries that are not provided as part of the repository:

- [**SDL3**](https://wiki.libsdl.org/SDL3/FrontPage) >= 3.2.6 for video, audio, input, and native dialogs.
- [**cpp-httplib**](https://github.com/yhirose/cpp-httplib) >= 0.23 as a HTTP client to download levels.
- [**glm**](https://github.com/g-truc/glm) providing useful additions to OpenGL.
- [**plog**](https://github.com/SergiusTheBest/plog) for logging
- [**zlib**](https://www.zlib.net/) as a compression utility
- [**gtest**](https://github.com/google/googletest) (optional) for testing.

You can supply these dependencies yourself via your system's library management tools, or the build system can locate the dependencies for you using [vcpkg](https://github.com/microsoft/vcpkg), a cross-platform dependency-management system developed by Microsoft. The official builds source their dependencies from vcpkg.

## Installing and using vcpkg

Set `VCPKG_ROOT` to a bootstrapped vcpkg checkout on every platform. An existing
Visual Studio installation may provide vcpkg, but the build still needs its
location in this environment variable. Otherwise clone
https://github.com/microsoft/vcpkg and run `bootstrap-vcpkg.bat` on Windows or
`./bootstrap-vcpkg.sh` on Linux/macOS.

The Linux `make.sh` wrapper selects vcpkg explicitly. Manual CMake builds use
it when `VCPKG_ROOT` is set; use `-DUSE_VCPKG=OFF` to select system libraries.
Do not embed an individual's game installation path in CMake defaults.

## Building - Windows

1. **Install the prerequisite build tools.**

    Make sure that you have Git and Visual Studio 2022 with the “Desktop development with C++” workload and the “C++ MFC for latest v143 build tools (x86 & x64)” component. If you don’t already have those installed or you aren’t sure, then open an elevated Command Prompt and run:
    <!--
    The following code block specifies the full path to the Visual Studio Installer because the Visual Studio Installer doesn’t add itself to the user’s Path. The installer is guaranteed to be in a specific location on 64-bit systems [1]. The installer will be in a different location on 32-bit systems [2], but Visual Studio 2022 doesn’t support 32-bit systems [3] so we can ignore that detail.

    [1]: <https://learn.microsoft.com/en-us/visualstudio/install/use-command-line-parameters-to-install-visual-studio?view=vs-2022>
    [2]: <https://github.com/microsoft/vswhere/wiki#installing>
    [3]: <https://learn.microsoft.com/en-us/answers/questions/1689898/does-visual-studio-build-tools-2022-support-32-bit>
    -->
    ```batch
    winget install Git.Git Microsoft.VisualStudio.2022.Community

    "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\setup.exe" modify^
        --passive^
        --channelId VisualStudio.17.Release^
        --productId Microsoft.VisualStudio.Product.Community^
        --add Microsoft.VisualStudio.Workload.NativeDesktop;includeRecommended^
        --add Microsoft.VisualStudio.Component.VC.ATLMFC
    ```

    **NOTE:**
    Run the commands below in a Visual Studio **x64 Native Tools Command Prompt**
    so the compiler is available. Ensure vcpkg is bootstrapped and set
    `VCPKG_ROOT` to its location; do not assume the prompt configured it.

2. **Clone the Descent3 source code.**

    Open a "x64 Native Tools Command Prompt" or "x64 Native Tools PowerShell" and run:
    ```batch
    git clone https://github.com/tiglate/Descent3.git
    ```

3. **Build Descent3.**

    ```batch
    cd Descent3
    set "VCPKG_ROOT=C:\path\to\vcpkg"
    cmake --preset win
    cmake --build --preset win --config Release
    ```
    Replace the example vcpkg path with your checkout. Use `--config Debug`
    instead for a Debug build; see [Build Options](#build-options).

Once CMake finishes, the built files will be put in `builds\win\build\Debug` or `builds\win\build\Release`.

## Building - macOS

1. **Install the prerequisite build tools.**

    * Make sure that [Xcode](https://developer.apple.com/xcode) is installed.
    * Make sure that [Homebrew](https://brew.sh) is installed.

2. **Acquire the library dependencies.**

    * To install the library dependencies:
        ```sh
        git clone https://github.com/microsoft/vcpkg
        cd vcpkg
        ./bootstrap-vcpkg.sh
        cd -
        export VCPKG_ROOT="$PWD/vcpkg"
        ```
        **NOTE:**
        You will need `$VCPKG_ROOT` defined in the environment for all build runs. It is a good idea to set this in your `.bashrc` or equivalent.
    * Create a custom VCPKG triplet that allows the mac build to run and create a universal binary.
      ```sh
      cd "$VCPKG_ROOT"/triplets
      cp ./arm64-osx.cmake ./community/universal-osx.cmake
      sed -i '' 's/^set(VCPKG_OSX_ARCHITECTURES.*$/set(VCPKG_OSX_ARCHITECTURES "arm64;x86_64")/' ./community/universal-osx.cmake
      ```

3. **Clone the Descent3 source code.**

    ```sh
    git clone https://github.com/tiglate/Descent3.git
    ```

4. **Build Descent3.**

    * Install the build tools.
      ```sh
      cd Descent3
      brew bundle install
      ```
    * Compile Descent 3.
      ```sh
      cmake --preset mac
      cmake --build --preset mac --config Release
      ```
      Use `--config Debug` instead for a Debug build. See [Build Options](#build-options).

Once CMake finishes, the built files will be put in `builds/mac/build/Debug` or `builds/mac/build/Release`.

## Building - Linux

1. **Install the prerequisite build tools.**

    * APT users (Debian, Ubuntu)
        ```sh
        sudo apt update
        sudo apt install -y --no-install-recommends git ninja-build make cmake g++ ca-certificates curl zip unzip tar
        ```
    * DNF users (Red Hat, Fedora)
        ```sh
        sudo dnf update --refresh
        sudo dnf install -y git ninja-build make cmake gcc-c++
        ```

2. **Acquire the library dependencies.**

    * We recommend using VCPKG to manage library dependencies.
        1. Clone vcpkg:
            ```sh
            git clone https://github.com/microsoft/vcpkg
            cd vcpkg
            ./bootstrap-vcpkg.sh
            cd -
            export VCPKG_ROOT="$PWD/vcpkg"
            ```
            **NOTE:**
            You will need `$VCPKG_ROOT` defined in the environment for all build runs. It is a good idea to set this in your `.bashrc` or equivalent.
        2. Install vcpkg-needed build tools and dependencies:
            * APT users
                ```sh
                sudo apt install -y --no-install-recommends curl pkg-config autoconf autoconf-archive automake libtool libltdl-dev make python3-jinja2 libx11-dev libxft-dev libxext-dev libwayland-dev libxkbcommon-dev libegl1-mesa-dev libibus-1.0-dev libasound2-dev libpulse-dev libaudio-dev libjack-dev libsndio-dev libxcursor-dev libxfixes-dev libxi-dev libxrandr-dev libxss-dev
                ```
            * DNF users
                ```sh
                sudo dnf install -y autoconf automake libtool perl-open perl-FindBin python-jinja2 libX11-devel libXft-devel libXext-devel wayland-devel libxkbcommon-devel mesa-libEGL-devel ibus-devel alsa-lib-devel pulseaudio-libs-devel
                ```
    * If you would like to manage the code dependencies yourself, install libraries and headers for the dependencies listed [above](#dependencies). SDL3 still being new at the time of writing, it may not be packaged by all popular non rolling-release Linux distributions yet.

3. **Clone the Descent3 source code.**

    ```sh
    git clone https://github.com/tiglate/Descent3.git
    ```

4. **Build and run using the wrapper.**

    ```sh
    cd Descent3
    make release
    make run ARGS="-windowed"
    ```

    The root Makefile forwards to `make.sh`. Plain `make` or `make build`
    compiles Debug; `make release` compiles Release. `make install` builds
    Release and installs to `~/Apps/Descent3`. `make run` builds, installs,
    and launches Release. First-run setup selects your commercial game assets.

    Wrapper outputs are in `build/build/Debug` or `build/build/Release`.
    Use `make help` or `./make.sh --help` to see all actions. Examples:

    ```sh
    make release ARGS="-j 4"
    make run ARGS='-assetdir "/path/to/Descent 3" -windowed'
    make install ARGS='--prefix "/path/to/install"'
    ```

    `ARGS` forwards options to the wrapper for build/install actions, or
    directly to the game for `run`. `make -j` does not set CMake's job count;
    use `ARGS="-j 4"` with a build action. `make run` always uses the wrapper's
    default install prefix; to run a custom installation, launch its executable
    directly. Do not use `sudo` for a user-local installation.

### Linux CMake preset alternative

To use system libraries or work directly with the provided preset instead of
the Linux wrapper:

```sh
cmake --preset linux
cmake --build --preset linux --config Release
```

Use `--config Debug` for Debug. These outputs are in
`builds/linux/build/Release` or `builds/linux/build/Debug`, separate from the
wrapper's `build/` tree. Add `-DUSE_VCPKG=OFF` to the configure command if using
system libraries. Never run `cmake .` in the source root.

## Tests

GoogleTest tests are disabled unless `BUILD_TESTING=ON`. For the Linux wrapper:

```sh
make configure
cmake -S . -B build -DBUILD_TESTING=ON
make release
ctest --test-dir build -C Release --output-on-failure
```

After enabling tests, `make test` builds Debug and runs its suite. For a
focused Release run, use
`ctest --test-dir build -C Release -R 'Control(Presets|Bindings)|GameAssets' --output-on-failure`.

With CMake presets, enable tests in the corresponding build tree. For example:

```sh
cmake --preset linux -DBUILD_TESTING=ON
cmake --build --preset linux --config Release
ctest --preset linux -C Release
```

Use `win` or `mac` instead of `linux` on those platforms. Match the test
configuration to the one you built.

## Cross-compilation

In order to cross-compile Descent3 to another platform (for example, Linux ARM64), you'll need to build auxiliary
tools natively which are needed to build data files. First create build-native directory and configure project in it:

```shell
mkdir build-native
cd build-native
cmake ..
cmake --build . --target HogMaker
```

Now, you are ready for cross-compilation. Create a new cross-compilation build directory and configure the project in it, but this time specify the location of the HogMaker native executable just built, as well as the [toolchain file](https://cmake.org/cmake/help/latest/manual/cmake-toolchains.7.html#id14) location. This enables the cross-compilation environment. An example toolchain file is provided for a Linux ARM64 build at `cmake/toolchains/linux-aarch64-gcc-toolchain.cmake`. The custom toolchain system can be used in combination with VCPKG to build all dependencies for the target system.

```shell
mkdir build-cross
cd build-cross
cmake -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/MyToolChain.cmake -DHogMaker_DIR=../build-native/ ..
cmake --build .
```

## Build Options

The Descent3 build can be customized by [setting CMake variables on the command line](https://cmake.org/cmake/help/latest/manual/cmake.1.html#cmdoption-cmake-D) during its "Configuration" phase (the command without the `--build` option). To set a variable, you prepend the variable name with `-D` and then append the value, all as one single parameter. For example:

```sh
cmake --preset linux -DBUILD_TESTING=ON
```

**NOTE:** CMake variables, or more technically _CMake cache entries_, will persist in their values until they are explicitly cleared. So, if you set a variable and then run another CMake command _without_ that variable specified, the variable will still be set. Variables must be explicitly unset, or the `builds/` directory cleaned, in order to be cleared.

You can also use [cmake-gui](https://cmake.org/cmake/help/latest/manual/cmake-gui.1.html)
to view or edit cache entries and generate build files.

| Option | Description | Default |
| --- | --- | --- |
| `CMAKE_BUILD_TYPE` | Configuration for single-config generators. Ninja Multi-Config (all provided presets and the wrapper) uses `--config Debug`, `Release`, or `RelWithDebInfo` at build time instead. | `Debug` for single-config builds |
| `CMAKE_INSTALL_PREFIX` | Installation directory; independent of the user preferences and saved asset location. | `<build-directory>/installed`; wrapper: `~/Apps/Descent3` |
| `BUILD_EDITOR` | Build the internal editor (Windows only). | `OFF` |
| `BUILD_TESTING` | Build GoogleTest tests and enable CTest discovery. | `OFF` |
| `DEFAULT_ADDITIONAL_DIRS` | Optional semicolon-separated, compiled-in read-only directories for packagers. See [base directories](USAGE.md#base-directories). Prefer runtime asset setup for individual installations. The wrapper clears this value. | Empty |
| `ENABLE_LOGGER` | Enable the legacy `LOGGER` compile definition. Runtime logging is configured with `-loglevel` and `-logfile`. | `OFF` |
| `ENABLE_MEM_RTL` | Use real-time-library memory management. | `ON` |
| `FATAL_GL_ERRORS` | Check OpenGL calls and raise exceptions on errors. | `OFF` |
| `FORCE_COLORED_OUTPUT` | Force colored diagnostics for supported compilers. | `OFF` |
| `FORCE_PORTABLE_INSTALL` | Put installed engine files together under the prefix. This does not relocate user preferences. | `ON` |
| `USE_VCPKG` | `DEFAULT` follows `VCPKG_ROOT`; `OFF` selects system dependencies. Set `VCPKG_ROOT` when requesting `ON`. | `DEFAULT`; wrapper: `ON` |
| `CODESIGN_IDENTITY` | macOS signing identity for dynamic libraries embedded in HOG files. | Empty |

Debug enables additional checks. Release optimizes gameplay performance;
RelWithDebInfo adds debug information to an optimized build.

For manual builds, `cmake --install <build-directory> --config Release` installs
the built engine. Run the build first: `cmake --install` does not compile it.
For packaging directories containing backslashes, quotes, or semicolons in
`DEFAULT_ADDITIONAL_DIRS`, remember that the value is emitted into a C++ string
literal; escape it accordingly.
