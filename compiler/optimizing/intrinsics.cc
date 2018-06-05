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
#include "driver/compiler_options.h"
#include "gc/space/image_space.h"
#include "image-inl.h"
#include "intrinsic_objects.h"
#include "nodes.h"
#include "obj_ptr-inl.h"
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

static ObjPtr<mirror::ObjectArray<mirror::Object>> GetBootImageLiveObjects()
    REQUIRES_SHARED(Locks::mutator_lock_) {
  gc::Heap* heap = Runtime::Current()->GetHeap();
  const std::vector<gc::space::ImageSpace*>& boot_image_spaces = heap->GetBootImageSpaces();
  DCHECK(!boot_image_spaces.empty());
  const ImageHeader& main_header = boot_image_spaces[0]->GetImageHeader();
  ObjPtr<mirror::ObjectArray<mirror::Object>> boot_image_live_objects =
      ObjPtr<mirror::ObjectArray<mirror::Object>>::DownCast(
          main_header.GetImageRoot<kWithoutReadBarrier>(ImageHeader::kBootImageLiveObjects));
  DCHECK(boot_image_live_objects != nullptr);
  DCHECK(heap->ObjectIsInBootImageSpace(boot_image_live_objects));
  return boot_image_live_objects;
}

static bool CheckIntegerCache(Thread* self,
                              ClassLinker* class_linker,
                              ObjPtr<mirror::ObjectArray<mirror::Object>> boot_image_live_objects,
                              ObjPtr<mirror::ObjectArray<mirror::Object>> boot_image_cache)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  DCHECK(boot_image_cache != nullptr);

  // Since we have a cache in the boot image, both java.lang.Integer and
  // java.lang.Integer$IntegerCache must be initialized in the boot image.
  ObjPtr<mirror::Class> cache_class = class_linker->LookupClass(
      self, "Ljava/lang/Integer$IntegerCache;", /* class_loader */ nullptr);
  DCHECK(cache_class != nullptr);
  DCHECK(cache_class->IsInitialized());
  ObjPtr<mirror::Class> integer_class =
      class_linker->LookupClass(self, "Ljava/lang/Integer;", /* class_loader */ nullptr);
  DCHECK(integer_class != nullptr);
  DCHECK(integer_class->IsInitialized());

  // Check that the current cache is the same as the `boot_image_cache`.
  ArtField* cache_field = cache_class->FindDeclaredStaticField("cache", "[Ljava/lang/Integer;");
  DCHECK(cache_field != nullptr);
  ObjPtr<mirror::ObjectArray<mirror::Object>> current_cache =
      ObjPtr<mirror::ObjectArray<mirror::Object>>::DownCast(cache_field->GetObject(cache_class));
  if (current_cache != boot_image_cache) {
    return false;  // Messed up IntegerCache.cache.
  }

  // Check that the range matches the boot image cache length.
  ArtField* low_field = cache_class->FindDeclaredStaticField("low", "I");
  DCHECK(low_field != nullptr);
  int32_t low = low_field->GetInt(cache_class);
  ArtField* high_field = cache_class->FindDeclaredStaticField("high", "I");
  DCHECK(high_field != nullptr);
  int32_t high = high_field->GetInt(cache_class);
  if (boot_image_cache->GetLength() != high - low + 1) {
    return false;  // Messed up IntegerCache.low or IntegerCache.high.
  }

  // Check that the elements match the boot image intrinsic objects and check their values as well.
  ArtField* value_field = integer_class->FindDeclaredInstanceField("value", "I");
  DCHECK(value_field != nullptr);
  for (int32_t i = 0, len = boot_image_cache->GetLength(); i != len; ++i) {
    ObjPtr<mirror::Object> boot_image_object =
        IntrinsicObjects::GetIntegerValueOfObject(boot_image_live_objects, i);
    DCHECK(Runtime::Current()->GetHeap()->ObjectIsInBootImageSpace(boot_image_object));
    // No need for read barrier for comparison with a boot image object.
    ObjPtr<mirror::Object> current_object =
        boot_image_cache->GetWithoutChecks<kVerifyNone, kWithoutReadBarrier>(i);
    if (boot_image_object != current_object) {
      return false;  // Messed up IntegerCache.cache[i]
    }
    if (value_field->GetInt(boot_image_object) != low + i) {
      return false;  // Messed up IntegerCache.cache[i].value.
    }
  }

  return true;
}

void IntrinsicVisitor::ComputeIntegerValueOfLocations(HInvoke* invoke,
                                                      CodeGenerator* codegen,
                                                      Location return_location,
                                                      Location first_argument_location) {
  if (codegen->GetCompilerOptions().IsBootImage()) {
    // TODO: Implement for boot image. We need access to CompilerDriver::IsImageClass()
    // to verify that the IntegerCache shall be in the image.
    return;
  }
  Runtime* runtime = Runtime::Current();
  gc::Heap* heap = runtime->GetHeap();
  if (heap->GetBootImageSpaces().empty()) {
    return;  // Running without boot image, cannot use required boot image objects.
  }

  // The intrinsic will call if it needs to allocate a j.l.Integer.
  LocationSummary::CallKind call_kind = LocationSummary::kCallOnMainOnly;
  {
    Thread* self = Thread::Current();
    ScopedObjectAccess soa(self);
    ObjPtr<mirror::ObjectArray<mirror::Object>> boot_image_live_objects = GetBootImageLiveObjects();
    ObjPtr<mirror::ObjectArray<mirror::Object>> cache =
        IntrinsicObjects::GetIntegerValueOfCache(boot_image_live_objects);
    if (cache == nullptr) {
      return;  // No cache in the boot image.
    }
    if (runtime->UseJitCompilation()) {
      if (!CheckIntegerCache(self, runtime->GetClassLinker(), boot_image_live_objects, cache)) {
        return;  // The cache was somehow messed up, probably by using reflection.
      }
    } else {
      DCHECK(runtime->IsAotCompiler());
      DCHECK(CheckIntegerCache(self, runtime->GetClassLinker(), boot_image_live_objects, cache));
      if (invoke->InputAt(0)->IsIntConstant()) {
        int32_t value = invoke->InputAt(0)->AsIntConstant()->GetValue();
        // Retrieve the `value` from the lowest cached Integer.
        ObjPtr<mirror::Object> low_integer =
            IntrinsicObjects::GetIntegerValueOfObject(boot_image_live_objects, 0u);
        ObjPtr<mirror::Class> integer_class =
            low_integer->GetClass<kVerifyNone, kWithoutReadBarrier>();
        ArtField* value_field = integer_class->FindDeclaredInstanceField("value", "I");
        DCHECK(value_field != nullptr);
        int32_t low = value_field->GetInt(low_integer);
        if (static_cast<uint32_t>(value) - static_cast<uint32_t>(low) <
            static_cast<uint32_t>(cache->GetLength())) {
          // No call, we shall use direct pointer to the Integer object. Note that we cannot
          // do this for JIT as the "low" can change through reflection before emitting the code.
          call_kind = LocationSummary::kNoCall;
        }
      }
    }
  }

  ArenaAllocator* allocator = invoke->GetBlock()->GetGraph()->GetAllocator();
  LocationSummary* locations = new (allocator) LocationSummary(invoke, call_kind, kIntrinsified);
  if (call_kind == LocationSummary::kCallOnMainOnly) {
    locations->SetInAt(0, Location::RegisterOrConstant(invoke->InputAt(0)));
    locations->AddTemp(first_argument_location);
    locations->SetOut(return_location);
  } else {
    locations->SetInAt(0, Location::ConstantLocation(invoke->InputAt(0)->AsConstant()));
    locations->SetOut(Location::RequiresRegister());
  }
}

static int32_t GetIntegerCacheLowFromIntegerCache(Thread* self)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  ObjPtr<mirror::Class> cache_class = Runtime::Current()->GetClassLinker()->LookupClass(
      self, "Ljava/lang/Integer$IntegerCache;", /* class_loader */ nullptr);
  DCHECK(cache_class != nullptr);
  DCHECK(cache_class->IsInitialized());
  ArtField* low_field = cache_class->FindDeclaredStaticField("low", "I");
  DCHECK(low_field != nullptr);
  return low_field->GetInt(cache_class);
}

static uint32_t CalculateBootImageOffset(ObjPtr<mirror::Object> object)
    REQUIRES_SHARED(Locks::mutator_lock_) {
  gc::Heap* heap = Runtime::Current()->GetHeap();
  DCHECK(heap->ObjectIsInBootImageSpace(object));
  return reinterpret_cast<const uint8_t*>(object.Ptr()) - heap->GetBootImageSpaces()[0]->Begin();
}

inline IntrinsicVisitor::IntegerValueOfInfo::IntegerValueOfInfo()
    : integer_boot_image_offset(0u),
      value_offset(0),
      low(0),
      length(0u),
      value_boot_image_offset(0u) {}

IntrinsicVisitor::IntegerValueOfInfo IntrinsicVisitor::ComputeIntegerValueOfInfo(HInvoke* invoke) {
  // Note that we could cache all of the data looked up here. but there's no good
  // location for it. We don't want to add it to WellKnownClasses, to avoid creating global
  // jni values. Adding it as state to the compiler singleton seems like wrong
  // separation of concerns.
  // The need for this data should be pretty rare though.

  // Note that at this point we can no longer abort the code generation. Therefore,
  // we need to provide data that shall not lead to a crash even if the fields were
  // modified through reflection since ComputeIntegerValueOfLocations() when JITting.

  Runtime* runtime = Runtime::Current();
  Thread* self = Thread::Current();
  ScopedObjectAccess soa(self);
  ObjPtr<mirror::ObjectArray<mirror::Object>> boot_image_live_objects = GetBootImageLiveObjects();
  ObjPtr<mirror::Object> low_integer =
      IntrinsicObjects::GetIntegerValueOfObject(boot_image_live_objects, 0u);
  ObjPtr<mirror::Class> integer_class = low_integer->GetClass<kVerifyNone, kWithoutReadBarrier>();
  ArtField* value_field = integer_class->FindDeclaredInstanceField("value", "I");
  DCHECK(value_field != nullptr);

  IntegerValueOfInfo info;
  info.integer_boot_image_offset = CalculateBootImageOffset(integer_class);
  info.value_offset = value_field->GetOffset().Uint32Value();
  if (runtime->UseJitCompilation()) {
    // Use the current `IntegerCache.low` for JIT to avoid truly surprising behavior if the
    // code messes up the `value` field in the lowest cached Integer using reflection.
    info.low = GetIntegerCacheLowFromIntegerCache(self);
  } else {
    // For AOT, the `low_integer->value` should be the same as `IntegerCache.low`.
    info.low = value_field->GetInt(low_integer);
    DCHECK_EQ(info.low, GetIntegerCacheLowFromIntegerCache(self));
  }
  // Do not look at `IntegerCache.high`, use the immutable length of the cache array instead.
  info.length = dchecked_integral_cast<uint32_t>(
      IntrinsicObjects::GetIntegerValueOfCache(boot_image_live_objects)->GetLength());

  if (invoke->InputAt(0)->IsIntConstant()) {
    int32_t input_value = invoke->InputAt(0)->AsIntConstant()->GetValue();
    uint32_t index = static_cast<uint32_t>(input_value) - static_cast<uint32_t>(info.low);
    if (index < static_cast<uint32_t>(info.length)) {
      ObjPtr<mirror::Object> integer =
          IntrinsicObjects::GetIntegerValueOfObject(boot_image_live_objects, index);
      DCHECK(runtime->GetHeap()->ObjectIsInBootImageSpace(integer));
      info.value_boot_image_offset = CalculateBootImageOffset(integer);
    } else {
      info.value_boot_image_offset = 0u;  // Not in the cache.
    }
  } else {
    info.array_data_boot_image_offset =
        CalculateBootImageOffset(boot_image_live_objects) +
        IntrinsicObjects::GetIntegerValueOfArrayDataOffset(boot_image_live_objects).Uint32Value();
  }

  return info;
}

}  // namespace art
