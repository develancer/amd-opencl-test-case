AMD OpenCL error test case
==========================

Reproducible on RX 5700 XT with ROCm (4.0.1) drivers under Ubuntu 20.04.1
with 5.4.0-65 kernel. And many other, I guess.

The code itself is a highly modified version of a part of code from johncantrell's
[bip39-solver-gpu](https://github.com/johncantrell97/bip39-solver-gpu), which, in turn,
is an OpenCL port of [secp256k1](https://github.com/bitcoin-core/secp256k1).

How to compile?
---------------

If you have ROCm or OpenCL libraries installed in a custom directory
(e.g. `/opt/rocm`) you'll have to modify the Makefile to reflect it. Simply replace
```
CFLAGS=-std=gnu99 -Wall -Wextra -Wno-deprecated-declarations -O3
```
with
```
CFLAGS=-std=gnu99 -Wall -Wextra -Wno-deprecated-declarations -O3 -I/opt/rocm/opencl/include
LDFLAGS=-L/opt/rocm/opencl/lib
```

Then, run
```
make
```

How to run?
-----------

```
USAGE: ./weird platform device [ compiler_flags ]
```
so, for example, if GPU card is device #1 on platform #1:
```
./weird 1 1
```

The correct computation result is
```
bbde464b6355ee6de6deba5ae860f8a66524937eee81dde224a0214efd795d09
```
but the 5700 XT produces
```
77262ca4b90e3fcb55f32ba92841024688802f53e75b16196c399de799377ba7
```
instead.

Running the kernel with no optimizations does not help,
it still produces incorrect results (different, though):
```
./weird 1 1 -cl-opt-disable
b155da6459a3e7f864a7c1217e83f35dadb2f74d2bd1afdb22901fbec1a10f53
```

Why?
----
It seems that AMD OpenCL compiler does not correctly handle overflow when
multiplying 32-bit integers cast to 64-bit. It results in the upper part of the result being truncated.
There may be some other issues as well.

The previous version of this bug report is on the branch `previous`.