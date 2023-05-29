# Sockets

Utility socket library.

## Testing

The Test directory is setup to work with [googletest](https://github.com/google/googletest).

## Building

![A1](https://github.com/chcly/Module.Sockets/actions/workflows/build-linux.yml/badge.svg)
![A2](https://github.com/chcly/Module.Sockets/actions/workflows/build-windows.yml/badge.svg)

```sh
cmake -S . -B Build -DSockets_BUILD_TEST=ON -DSockets_AUTO_RUN_TEST=ON
cmake --build Build
```

### Optional defines

| Option                     | Description                                          | Default |
| :------------------------- | :--------------------------------------------------- | :-----: |
| Sockets_BUILD_TEST         | Build the unit test program.                         |   ON    |
| Sockets_AUTO_RUN_TEST      | Automatically run the test program.                  |   OFF   |
| Sockets_USE_STATIC_RUNTIME | Build with the MultiThreaded(Debug) runtime library. |   ON    |
