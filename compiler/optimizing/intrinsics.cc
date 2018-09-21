/*
 * Copyright (C) 2015 The Android Open Source Project
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

#include "intrinsics.h"

#include "art_field-inl.h"
#include "art_method-inl.h"
#include "base/utils.h"
#include "class_linker.h"
#include "dex/invoke_type.h"
#include "driver/compiler_driver.h"
#include "driver/compiler_options.h"
#include "mirror/dex_cache-inl.h"
#include "nodes.h"
#include "scoped_thread_state_change-inl.h"
#include "thread-current-inl.h"

namespace art {

std::ostream& operator<<(std::ostream& os, const Intrinsics& intrinsic) {
  switch (intrinsic) {
    case Intrinsics::kNone:
      os << "None";
      break;
#define OPTIMIZING_INTRINSICS(Name, IsStatic, NeedsEnvironmentOrCache, SideEffects, Exceptions, ...) \
    case Intrinsics::k ## Name: \
      os << # Name; \
      break;
#include "intrinsics_list.h"
      INTRINSICS_LIST(OPTIMIZING_INTRINSICS)
#undef STATIC_INTRINSICS_LIST
#undef VIRTUAL_INTRINSICS_LIST
#undef OPTIMIZING_INTRINSICS
  }
  return os;
}

void IntrinsicVisitor::ComputeIntegerValueOfLocations(HInvoke* invoke,
                                                      CodeGenerator* codegen,
                                                      Location return_location,
                                                      Location first_argument_location) {
  if (Runtime::Current()->IsAotCompiler()) {
    if (codegen->GetCompilerOptions().IsBootImage() ||
        codegen->GetCompilerOptions().GetCompilePic()) {
      // TODO(ngeoffray): Support boot image compilation.
      return;
    }
  }

  IntegerValueOfInfo info = ComputeIntegerValueOfInfo();

  // Most common case is that we have found all we needed (classes are initialized
  // and in the boot image). Bail if not.
  if (info.integer_cache == nullptr ||
      info.integer == nullptr ||
      info.cache == nullptr ||
      info.value_offset == 0 ||
      // low and high cannot be 0, per the spec.
      info.low == 0 ||
      info.high == 0) {
    LOG(INFO) << "Integer.valueOf will not be optimized";
    return;
  }

  // The intrinsic will call if it needs to allocate a j.l.Integer.
  LocationSummary* locations = new (invoke->GetBlock()->GetGraph()->GetAllocator()) LocationSummary(
      invoke, LocationSummary::kCallOnMainOnly, kIntrinsified);
  if (!invoke->InputAt(0)->IsConstant()) {
    locations->SetInAt(0, Location::RequiresRegister());
  }
  locations->AddTemp(first_argument_location);
  locations->SetOut(return_location);
}

IntrinsicVisitor::IntegerValueOfInfo IntrinsicVisitor::ComputeIntegerValueOfInfo() {
  // Note that we could cache all of the data looked up here. but there's no good
  // location for it. We don't want to add it to WellKnownClasses, to avoid creating global
  // jni values. Adding it as state to the compiler singleton seems like wrong
  // separation of concerns.
  // The need for this data should be pretty rare though.

  // The most common case is that the classes are in the boot image and initialized,
  // which is easy to generate code for. We bail if not.
  Thread* self = Thread::Current();
  ScopedObjectAccess soa(self);
  Runtime* runtime = Runtime::Current();
  ClassLinker* class_linker = runtime->GetClassLinker();
  gc::Heap* heap = runtime->GetHeap();
  IntegerValueOfInfo info;
  info.integer_cache = class_linker->FindSystemClass(self, "Ljava/lang/Integer$IntegerCache;");
  if (info.integer_cache == nullptr) {
    self->ClearException();
    return info;
  }
  if (!heap->ObjectIsInBootImageSpace(info.integer_cache) || !info.integer_cache->IsInitialized()) {
    // Optimization only works if the class is initialized and in the boot image.
    return info;
  }
  info.integer = class_linker->FindSystemClass(self, "Ljava/lang/Integer;");
  if (info.integer == nullptr) {
    self->ClearException();
    return info;
  }
  if (!heap->ObjectIsInBootImageSpace(info.integer) || !info.integer->IsInitialized()) {
    // Optimization only works if the class is initialized and in the boot image.
    return info;
  }

  ArtField* field = info.integer_cache->FindDeclaredStaticField("cache", "[Ljava/lang/Integer;");
  if (field == nullptr) {
    return info;
  }
  info.cache = static_cast<mirror::ObjectArray<mirror::Object>*>(
      field->GetObject(info.integer_cache).Ptr());
  if (info.cache == nullptr) {
    return info;
  }

  if (!heap->ObjectIsInBootImageSpace(info.cache)) {
    // Optimization only works if the object is in the boot image.
    return info;
  }

  field = info.integer->FindDeclaredInstanceField("value", "I");
  if (field == nullptr) {
    return info;
  }
  info.value_offset = field->GetOffset().Int32Value();

  field = info.integer_cache->FindDeclaredStaticField("low", "I");
  if (field == nullptr) {
    return info;
  }
  info.low = field->GetInt(info.integer_cache);

  field = info.integer_cache->FindDeclaredStaticField("high", "I");
  if (field == nullptr) {
    return info;
  }
  info.high = field->GetInt(info.integer_cache);

  DCHECK_EQ(info.cache->GetLength(), info.high - info.low + 1);
  return info;
}

}  // namespace art
