/*
 * Descent 3
 * Copyright (C) 2026 Descent Developers
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ssl_lib.h"

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

int paged_in_count = 0;
int paged_in_num = 0;

namespace {

using Bytes = std::vector<uint8_t>;

void Append16(Bytes &bytes, uint16_t value) {
  bytes.push_back(static_cast<uint8_t>(value));
  bytes.push_back(static_cast<uint8_t>(value >> 8));
}

void Append32(Bytes &bytes, uint32_t value) {
  Append16(bytes, static_cast<uint16_t>(value));
  Append16(bytes, static_cast<uint16_t>(value >> 16));
}

void Set32(Bytes &bytes, size_t offset, uint32_t value) {
  for (size_t i = 0; i < 4; ++i)
    bytes[offset + i] = static_cast<uint8_t>(value >> (i * 8));
}

Bytes Format(uint16_t bits = 16, uint16_t channels = 1, uint32_t rate = 22050) {
  Bytes bytes;
  Append16(bytes, 1); // PCM
  Append16(bytes, channels);
  Append32(bytes, rate);
  Append32(bytes, rate * channels * (bits / 8));
  Append16(bytes, channels * (bits / 8));
  Append16(bytes, bits);
  return bytes;
}

Bytes Samples16() {
  Bytes bytes;
  for (int16_t sample : {-12000, 0, 12000})
    Append16(bytes, static_cast<uint16_t>(sample));
  return bytes;
}

void Chunk(Bytes &wave, const std::string &type, const Bytes &payload) {
  wave.insert(wave.end(), type.begin(), type.end());
  Append32(wave, static_cast<uint32_t>(payload.size()));
  wave.insert(wave.end(), payload.begin(), payload.end());
  if (payload.size() % 2)
    wave.push_back(0); // RIFF chunks are word-aligned.
}

Bytes Wave() { return {'R', 'I', 'F', 'F', 0, 0, 0, 0, 'W', 'A', 'V', 'E'}; }

class WaveLoaderTest : public testing::Test {
protected:
  void SetUp() override {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    directory_ = std::filesystem::temp_directory_path() / ("descent3-wave-test-" + std::to_string(stamp));
    ASSERT_TRUE(std::filesystem::create_directory(directory_));
    SoundFiles[0] = {};
    SoundFiles[0].used = 1; // The caller reserves the slot before loading.
  }

  void TearDown() override {
    SoundLoadFree(0);
    SoundFiles[0] = {};
    std::error_code ec;
    std::filesystem::remove_all(directory_, ec);
  }

  int Load(Bytes wave, bool high_quality = true, bool load_samples = true) {
    Set32(wave, 4, static_cast<uint32_t>(wave.size() - 8));
    return LoadRaw(wave, high_quality, load_samples);
  }

  int LoadRaw(const Bytes &wave, bool high_quality = true, bool load_samples = true) {
    const auto path = directory_ / "fixture.wav";
    {
      std::ofstream output(path, std::ios::binary);
      output.write(reinterpret_cast<const char *>(wave.data()), static_cast<std::streamsize>(wave.size()));
      if (!output.good())
        return -1;
    }
    return SoundLoadWaveFile(path.string().c_str(), 2.0f, 0, high_quality, load_samples, &error_);
  }

  void ExpectNoSamples() {
    EXPECT_EQ(SoundFiles[0].sample_8bit, nullptr);
    EXPECT_EQ(SoundFiles[0].sample_16bit, nullptr);
    EXPECT_EQ(SoundFiles[0].sample_length, 0);
  }

  std::filesystem::path directory_;
  int error_ = -1;
};

TEST_F(WaveLoaderTest, FormatWithoutDataIsRejected) {
  auto wave = Wave();
  Chunk(wave, "fmt ", Format());
  EXPECT_EQ(Load(wave), 0);
  EXPECT_EQ(error_, 1);
  ExpectNoSamples();
}

TEST_F(WaveLoaderTest, EmptyWaveIsRejected) {
  EXPECT_EQ(Load(Wave()), 0);
  ExpectNoSamples();
}

TEST_F(WaveLoaderTest, MissingFormatIsRejected) {
  auto wave = Wave();
  Chunk(wave, "data", Samples16());
  EXPECT_EQ(Load(wave), 0);
  ExpectNoSamples();
}

TEST_F(WaveLoaderTest, InvalidRiffIsRejected) {
  auto wave = Wave();
  wave[0] = 'X';
  EXPECT_EQ(Load(wave), 0);
  ExpectNoSamples();
}

TEST_F(WaveLoaderTest, MissingFileReportsNoFile) {
  const auto missing = directory_ / "missing.wav";
  EXPECT_EQ(SoundLoadWaveFile(missing.string().c_str(), 1.0f, 0, true, true, &error_), 0);
  EXPECT_EQ(error_, 0);
  ExpectNoSamples();
}

TEST_F(WaveLoaderTest, MetadataOnlyLoadDoesNotAllocateSamples) {
  auto wave = Wave();
  Chunk(wave, "fmt ", Format());
  Chunk(wave, "data", Samples16());
  EXPECT_EQ(Load(wave, true, false), 1);
  ExpectNoSamples();
}

TEST_F(WaveLoaderTest, Loads16BitSamplesWithExistingAlignmentAndVolume) {
  auto wave = Wave();
  Chunk(wave, "fmt ", Format());
  Chunk(wave, "data", Samples16());
  ASSERT_EQ(Load(wave), 1);
  const auto &sound = SoundFiles[0];
  ASSERT_NE(sound.sample_16bit, nullptr);
  EXPECT_EQ(sound.sample_8bit, nullptr);
  EXPECT_EQ(sound.np_sample_length, 3);
  ASSERT_EQ(sound.sample_length, 5);
  EXPECT_EQ(sound.sample_16bit[0], -12000);
  EXPECT_EQ(sound.sample_16bit[1], 0);
  EXPECT_EQ(sound.sample_16bit[2], 12000);
  EXPECT_EQ(sound.sample_16bit[3], 0);
  EXPECT_EQ(sound.sample_16bit[4], 0);
}

TEST_F(WaveLoaderTest, Loads8BitSamplesWithExistingAlignmentAndVolume) {
  auto wave = Wave();
  Chunk(wave, "fmt ", Format(8));
  Chunk(wave, "data", {0, 128, 255});
  ASSERT_EQ(Load(wave, false), 1);
  const auto &sound = SoundFiles[0];
  ASSERT_NE(sound.sample_8bit, nullptr);
  EXPECT_EQ(sound.sample_16bit, nullptr);
  EXPECT_EQ(sound.np_sample_length, 3);
  ASSERT_EQ(sound.sample_length, 5);
  EXPECT_EQ(sound.sample_8bit[0], 0);
  EXPECT_EQ(sound.sample_8bit[1], 128);
  EXPECT_EQ(sound.sample_8bit[2], 255);
  EXPECT_EQ(sound.sample_8bit[3], 128);
  EXPECT_EQ(sound.sample_8bit[4], 128);
}

TEST_F(WaveLoaderTest, Converts8BitSamplesToHighQuality) {
  auto wave = Wave();
  Chunk(wave, "fmt ", Format(8));
  Chunk(wave, "data", {0, 128, 255});
  ASSERT_EQ(Load(wave), 1);
  ASSERT_NE(SoundFiles[0].sample_16bit, nullptr);
  EXPECT_EQ(SoundFiles[0].sample_8bit, nullptr);
  EXPECT_EQ(SoundFiles[0].sample_16bit[0], -32768);
  EXPECT_EQ(SoundFiles[0].sample_16bit[1], 0);
  EXPECT_EQ(SoundFiles[0].sample_16bit[2], 32512);
}

TEST_F(WaveLoaderTest, Converts16BitSamplesToLowQuality) {
  auto wave = Wave();
  Chunk(wave, "fmt ", Format());
  Chunk(wave, "data", Samples16());
  ASSERT_EQ(Load(wave, false), 1);
  ASSERT_NE(SoundFiles[0].sample_8bit, nullptr);
  EXPECT_EQ(SoundFiles[0].sample_16bit, nullptr);
  EXPECT_EQ(SoundFiles[0].sample_8bit[0], 81);
  EXPECT_EQ(SoundFiles[0].sample_8bit[1], 127);
  EXPECT_EQ(SoundFiles[0].sample_8bit[2], 174);
}

TEST_F(WaveLoaderTest, Repeated16BitWaveformsKeepOnlyLastData) {
  auto wave = Wave();
  Chunk(wave, "fmt ", Format());
  Chunk(wave, "data", Samples16());
  Chunk(wave, "fmt ", Format());
  Bytes last;
  Append16(last, 6000);
  Chunk(wave, "data", last);
  ASSERT_EQ(Load(wave), 1);
  ASSERT_NE(SoundFiles[0].sample_16bit, nullptr);
  EXPECT_EQ(SoundFiles[0].np_sample_length, 1);
  EXPECT_EQ(SoundFiles[0].sample_length, 1);
  EXPECT_EQ(SoundFiles[0].sample_16bit[0], 6000);
}

TEST_F(WaveLoaderTest, Repeated8BitWaveformsKeepOnlyLastData) {
  auto wave = Wave();
  Chunk(wave, "fmt ", Format(8));
  Chunk(wave, "data", {0, 128, 255});
  Chunk(wave, "fmt ", Format(8));
  Chunk(wave, "data", {200});
  ASSERT_EQ(Load(wave, false), 1);
  ASSERT_NE(SoundFiles[0].sample_8bit, nullptr);
  EXPECT_EQ(SoundFiles[0].np_sample_length, 1);
  EXPECT_EQ(SoundFiles[0].sample_length, 1);
  EXPECT_EQ(SoundFiles[0].sample_8bit[0], 200);
}

TEST_F(WaveLoaderTest, Later16BitWaveformDiscardsEarlier8BitData) {
  auto wave = Wave();
  Chunk(wave, "fmt ", Format(8));
  Chunk(wave, "data", {255});
  Chunk(wave, "fmt ", Format());
  Chunk(wave, "data", Samples16());
  ASSERT_EQ(Load(wave), 1);
  EXPECT_EQ(SoundFiles[0].sample_8bit, nullptr);
  ASSERT_NE(SoundFiles[0].sample_16bit, nullptr);
  EXPECT_EQ(SoundFiles[0].np_sample_length, 3);
  EXPECT_EQ(SoundFiles[0].sample_16bit[0], -12000);
}

TEST_F(WaveLoaderTest, Later8BitWaveformDiscardsEarlier16BitData) {
  auto wave = Wave();
  Chunk(wave, "fmt ", Format());
  Chunk(wave, "data", Samples16());
  Chunk(wave, "fmt ", Format(8));
  Chunk(wave, "data", {200});
  ASSERT_EQ(Load(wave, false), 1);
  EXPECT_EQ(SoundFiles[0].sample_16bit, nullptr);
  ASSERT_NE(SoundFiles[0].sample_8bit, nullptr);
  EXPECT_EQ(SoundFiles[0].np_sample_length, 1);
  EXPECT_EQ(SoundFiles[0].sample_8bit[0], 200);
}

TEST_F(WaveLoaderTest, InvalidLaterWaveformDoesNotPublishPartialSamples) {
  auto wave = Wave();
  Chunk(wave, "fmt ", Format());
  Chunk(wave, "data", Samples16());
  Chunk(wave, "fmt ", Format());
  Chunk(wave, "data", {1, 2, 3}); // Incomplete 16-bit sample.
  EXPECT_EQ(Load(wave), 0);
  ExpectNoSamples();
}

TEST_F(WaveLoaderTest, FailedReloadPreservesPreviouslyLoadedSamples) {
  auto wave = Wave();
  Chunk(wave, "fmt ", Format());
  Chunk(wave, "data", Samples16());
  ASSERT_EQ(Load(wave), 1);
  auto *previous = SoundFiles[0].sample_16bit;
  Chunk(wave, "fmt ", Format());
  Chunk(wave, "data", {1, 2, 3});
  EXPECT_EQ(Load(wave), 0);
  ASSERT_EQ(SoundFiles[0].sample_16bit, previous);
  EXPECT_EQ(SoundFiles[0].sample_length, 5);
  EXPECT_EQ(SoundFiles[0].np_sample_length, 3);
  EXPECT_EQ(previous[0], -12000);
}

TEST_F(WaveLoaderTest, SuccessfulReloadReplacesPreviouslyLoadedSamples) {
  auto wave = Wave();
  Chunk(wave, "fmt ", Format());
  Chunk(wave, "data", Samples16());
  ASSERT_EQ(Load(wave), 1);
  wave = Wave();
  Chunk(wave, "fmt ", Format(8));
  Chunk(wave, "data", {200});
  ASSERT_EQ(Load(wave, false), 1);
  EXPECT_EQ(SoundFiles[0].sample_16bit, nullptr);
  ASSERT_NE(SoundFiles[0].sample_8bit, nullptr);
  EXPECT_EQ(SoundFiles[0].sample_8bit[0], 200);
}

TEST_F(WaveLoaderTest, ChunkLengthPastRiffBoundaryIsRejectedBeforeAllocation) {
  auto wave = Wave();
  Chunk(wave, "fmt ", Format());
  Chunk(wave, "data", Samples16());
  Set32(wave, 40, 0xfffffffe); // data length after the 16-byte PCM fmt chunk.
  EXPECT_EQ(Load(wave), 0);
  ExpectNoSamples();
}

TEST_F(WaveLoaderTest, ShortFormatChunkIsRejected) {
  auto wave = Wave();
  auto format = Format();
  format.resize(14);
  Chunk(wave, "fmt ", format);
  Chunk(wave, "data", Samples16());
  EXPECT_EQ(Load(wave), 0);
  ExpectNoSamples();
}

TEST_F(WaveLoaderTest, TruncatedChunkHeaderIsRejected) {
  auto wave = Wave();
  wave.insert(wave.end(), {'f', 'm', 't', ' '});
  EXPECT_EQ(Load(wave), 0);
  ExpectNoSamples();
}

TEST_F(WaveLoaderTest, TruncatedRiffHeaderIsRejected) {
  auto wave = Wave();
  wave.resize(10);
  EXPECT_EQ(LoadRaw(wave), 0);
  ExpectNoSamples();
}

TEST_F(WaveLoaderTest, InvalidRiffSizesAreRejectedWithoutOverflow) {
  for (uint32_t size : {3u, 1000u, 0xffffffffu}) {
    auto wave = Wave();
    Set32(wave, 4, size);
    EXPECT_EQ(LoadRaw(wave), 0);
    ExpectNoSamples();
  }
}

TEST_F(WaveLoaderTest, OddLength16BitDataIsRejected) {
  auto wave = Wave();
  Chunk(wave, "fmt ", Format());
  Chunk(wave, "data", {1, 2, 3});
  EXPECT_EQ(Load(wave), 0);
  ExpectNoSamples();
}

TEST_F(WaveLoaderTest, SkipsPaddingAfterOddUnknownChunks) {
  auto wave = Wave();
  Chunk(wave, "JUNK", {1, 2, 3});
  Chunk(wave, "fmt ", Format());
  Chunk(wave, "data", Samples16());
  ASSERT_EQ(Load(wave), 1);
  ASSERT_NE(SoundFiles[0].sample_16bit, nullptr);
  EXPECT_EQ(SoundFiles[0].sample_16bit[0], -12000);
}

TEST_F(WaveLoaderTest, AcceptsLegacyFinal8BitDataWithoutRiffPadding) {
  auto wave = Wave();
  Chunk(wave, "fmt ", Format(8));
  Chunk(wave, "data", {200});
  wave.pop_back();
  ASSERT_EQ(Load(wave, false), 1);
  ASSERT_NE(SoundFiles[0].sample_8bit, nullptr);
  EXPECT_EQ(SoundFiles[0].sample_8bit[0], 200);
}

} // namespace
