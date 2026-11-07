#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <type_traits>

// Exercise the same standalone native implementation used by imResampleMex.
#include "../channels/private/imResampleMex.cpp"

static int failures = 0;
static int cases = 0;

template<typename T>
void checkCase(int outputHeight, int outputWidth, int horizontalFactor,
               int channels, int inputOffset, int outputOffset, T norm) {
  const int inputHeight = 2 * outputHeight;
  const int inputWidth = horizontalFactor * outputWidth;
  const int inputCount = inputHeight * inputWidth * channels;
  const int outputCount = outputHeight * outputWidth * channels;
  const int guard = 8;
  const T sentinel = T(-123456);
  T *inputStorage = static_cast<T *>(alMalloc((inputCount + guard + inputOffset) * sizeof(T), 16));
  T *outputStorage = static_cast<T *>(alMalloc((outputCount + 2 * guard + outputOffset) * sizeof(T), 16));
  T *input = inputStorage + inputOffset;
  T *output = outputStorage + guard + outputOffset;
  std::fill(outputStorage, outputStorage + outputCount + 2 * guard + outputOffset, sentinel);
  std::fill(output, output + outputCount, T(0));
  for (int i = 0; i < inputCount; ++i) input[i] = T((i * 13) % 251) / T(8);

  resample(input, output, inputHeight, outputHeight, inputWidth, outputWidth, channels, norm);

  bool ok = true;
  for (int i = 0; i < guard + outputOffset; ++i)
    ok = ok && outputStorage[i] == sentinel;
  for (int i = 0; i < guard; ++i)
    ok = ok && output[outputCount + i] == sentinel;

  // At integer scale factors the resizer averages each source rectangle.
  // Preserve its existing small normalization correction in the oracle.
  const double correctedNorm = double(norm) / double(T(1 + 1e-6));
  const double tolerance = std::is_same<T, float>::value ? 2e-6 : 1e-12;
  for (int channel = 0; channel < channels; ++channel) {
    for (int x = 0; x < outputWidth; ++x) {
      for (int y = 0; y < outputHeight; ++y) {
        double total = 0;
        for (int dx = 0; dx < horizontalFactor; ++dx)
          for (int dy = 0; dy < 2; ++dy)
            total += input[channel * inputHeight * inputWidth +
                           (horizontalFactor * x + dx) * inputHeight + 2 * y + dy];
        const double expected = total / (2 * horizontalFactor) * correctedNorm;
        const double actual = output[channel * outputHeight * outputWidth + x * outputHeight + y];
        ok = ok && std::isfinite(actual) &&
             std::abs(actual - expected) <= tolerance * std::max(1.0, std::abs(expected));
      }
    }
  }
  ++cases;
  if (!ok) {
    if (failures < 8)
      std::fprintf(stderr, "%s: output=%dx%d, width factor=%d, channels=%d, offsets=%d/%d, norm=%g\n",
                   std::is_same<T, float>::value ? "float" : "double",
                   outputHeight, outputWidth, horizontalFactor, channels,
                   inputOffset, outputOffset, double(norm));
    ++failures;
  }
  alFree(inputStorage);
  alFree(outputStorage);
}

template<typename T>
void checkType() {
  const int offsets = 16 / sizeof(T);
  for (int height = 1; height <= 9; ++height)
    for (int width = 1; width <= 7; ++width)
      for (int factor = 1; factor <= 4; ++factor)
        for (int channels = 1; channels <= 3; channels += 2)
          for (int inputOffset = 0; inputOffset < offsets; ++inputOffset)
            for (int outputOffset = 0; outputOffset < offsets; ++outputOffset) {
              checkCase<T>(height, width, factor, channels, inputOffset, outputOffset, T(1));
              checkCase<T>(height, width, factor, channels, inputOffset, outputOffset, T(0.5));
            }
}

int main() {
  checkType<float>();
  checkType<double>();
  std::printf("%d/%d cases passed\n", cases - failures, cases);
  return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
