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
USAGE: ./weird platform device [ seed ]
```
so, for example, if GPU card is device #1 on platform #1:
```
./weird 1 1
```

Seed is optional — if not given, defaults to 42.

The correct computation result for the default seed value is
```
8c7b0713bd8315dcb8304433f3df42867af832b3229b3450fd2559ca8e36ee23 42
```
but the 5700 XT produces
```
6c117d8e1ae859eb0a6ce04158276dc2827da9197a4d9c937de1707593252e12 42
```
instead.

Why?
----
It seems that AMD OpenCL compiler does not correctly handle overflow when
multiplying 32-bit integers cast to 64-bit. It results in the upper part of the result being truncated.

The “fixed” version, with all 32-to-64-bit multiplications replaced with a custom function
```C
ulong mul(uint x, uint y) {
  const uint xH = x >> 16, xL = x & 0xFFFFu;
  const uint yH = y >> 16, yL = y & 0xFFFFu;

  ulong z = xH * yH;
  z <<= 16;
  z += xH * yL;
  z += xL * yH;
  z <<= 16;
  z += xL * yL;
  return z;
}
```
is on a branch named `fix`, and gives correct results. Sooo much slower, though.
