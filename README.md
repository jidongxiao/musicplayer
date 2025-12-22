# Build Instructions

## Command Line Only (WSL/Linux)

```console
mkdir build && cd build
cmake -DCLI_ONLY=ON ..
make
```

Run the CLI version:

```console
./player --cli
```

## GUI (Linux)

```console
mkdir build && cd build
cmake ..
make
```

Run the GUI version:

```console
./player --gui
```

or

```console
./player          # launches GUI if no argument is provided
```

## GUI Version (Windows with Qt Creator)

- Open the project in Qt Creator.
- Build the project.
- Click Run to launch the GUI.
