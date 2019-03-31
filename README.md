# libamsterdam

[![Travis CI][shields.io]][travis-ci.com]

libamsterdam is an implementation of asynchronous channels for C++17.

## Usage

```c++
#include <cassert>

#include <amsterdam.h>

int main() {
    auto [tx, rx] = amst::channel<int>();

    tx.push(5);
    assert(tx.pop() == 5);
}
```

## Installation

```sh
git clone https://github.com/Gregory-Meyer/amsterdam.git
mkdir amsterdam/build
cd amsterdam/build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . -j `nproc`
cmake --build . --target test
sudo cmake --build . --target install
```

## License

libamsterdam is MIT licensed.

[travis-ci.com]: https://travis-ci.com/Gregory-Meyer/amsterdam/
[shields.io]: https://img.shields.io/travis/com/Gregory-Meyer/amsterdam.svg
