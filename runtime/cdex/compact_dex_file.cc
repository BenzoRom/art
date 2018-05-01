/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "compact_dex_file.h"

namespace art {

constexpr uint8_t CompactDexFile::kDexMagic[kDexMagicSize];
constexpr uint8_t CompactDexFile::kDexMagicVersion[];

void CompactDexFile::WriteMagic(uint8_t* magic) {
  std::copy_n(kDexMagic, kDexMagicSize, magic);
}

void CompactDexFile::WriteCurrentVersion(uint8_t* magic) {
  std::copy_n(kDexMagicVersion, kDexVersionLen, magic + kDexMagicSize);
}

bool CompactDexFile::IsMagicValid(const uint8_t* magic) {
  return (memcmp(magic, kDexMagic, sizeof(kDexMagic)) == 0);
}

bool CompactDexFile::IsVersionValid(const uint8_t* magic) {
  const uint8_t* version = &magic[sizeof(kDexMagic)];
  return memcmp(version, kDexMagicVersion, kDexVersionLen) == 0;
}

bool CompactDexFile::IsMagicValid() const {
  return IsMagicValid(header_->magic_);
}

bool CompactDexFile::IsVersionValid() const {
  return IsVersionValid(header_->magic_);
}

}  // namespace art
