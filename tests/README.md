# Native image-resampling regression test

This test includes the production C++ resizer directly, following the existing
standalone channels approach. It needs an SSE2-capable C++11 compiler, not MATLAB.
From the repository root:

```sh
c++ -std=c++11 -O2 tests/im_resample_short_output.cpp -o /tmp/im-resample-test
/tmp/im-resample-test
```

For bounds and undefined-behavior checks:

```sh
c++ -std=c++11 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  tests/im_resample_short_output.cpp -o /tmp/im-resample-test-sanitized
ASAN_OPTIONS=detect_leaks=0 /tmp/im-resample-test-sanitized
```

Leak detection is disabled in the command above because some supervised runtimes
cannot run LeakSanitizer. AddressSanitizer and UndefinedBehaviorSanitizer remain
active.

The matrix covers float and double inputs, every element-aligned offset modulo
16 bytes, one- and three-channel images, output heights 1–9, widths 1–7, horizontal
integer reduction factors 1–4, and two normalization values. It verifies output
guards and the numerical average of each source rectangle. The short-height
cases reproduce the alignment-prefix overwrite; larger heights cover scalar and
SSE paths. This is native core coverage, not a MATLAB/MEX integration or complete
toolbox test.
