/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef ART_RUNTIME_ATOMIC_H_
#define ART_RUNTIME_ATOMIC_H_

#include <stdint.h>
#include <atomic>
#include <limits>
#include <vector>

#include "arch/instruction_set.h"
#include "base/logging.h"
#include "base/macros.h"

namespace art {

class Mutex;

// QuasiAtomic encapsulates two separate facilities that we are
// trying to move away from:  "quasiatomic" 64 bit operations
// and custom memory fences.  For the time being, they remain
// exposed.  Clients should be converted to use either class Atomic
// below whenever possible, and should eventually use C++11 atomics.
// The two facilities that do not have a good C++11 analog are
// ThreadFenceForConstructor and Atomic::*JavaData.
//
// NOTE: Two "quasiatomic" operations on the exact same memory address
// are guaranteed to operate atomically with respect to each other,
// but no guarantees are made about quasiatomic operations mixed with
// non-quasiatomic operations on the same address, nor about
// quasiatomic operations that are performed on partially-overlapping
// memory.
class QuasiAtomic {
  static constexpr bool NeedSwapMutexes(InstructionSet isa) {
    // TODO - mips64 still need this for Cas64 ???
    return (isa == InstructionSet::kMips) || (isa == InstructionSet::kMips64);
  }

 public:
  static void Startup();

  static void Shutdown();

  // Reads the 64-bit value at "addr" without tearing.
  static int64_t Read64(volatile const int64_t* addr) {
    if (!NeedSwapMutexes(kRuntimeISA)) {
      int64_t value;
#if defined(__LP64__)
      value = *addr;
#else
#if defined(__arm__)
#if defined(__ARM_FEATURE_LPAE)
      // With LPAE support (such as Cortex-A15) then ldrd is defined not to tear.
      __asm__ __volatile__("@ QuasiAtomic::Read64\n"
        "ldrd     %0, %H0, %1"
        : "=r" (value)
        : "m" (*addr));
#else
      // Exclusive loads are defined not to tear, clearing the exclusive state isn't necessary.
      __asm__ __volatile__("@ QuasiAtomic::Read64\n"
        "ldrexd     %0, %H0, %1"
        : "=r" (value)
        : "Q" (*addr));
#endif
#elif defined(__i386__)
  __asm__ __volatile__(
      "movq     %1, %0\n"
      : "=x" (value)
      : "m" (*addr));
#else
      LOG(FATAL) << "Unsupported architecture";
#endif
#endif  // defined(__LP64__)
      return value;
    } else {
      return SwapMutexRead64(addr);
    }
  }

  // Writes to the 64-bit value at "addr" without tearing.
  static void Write64(volatile int64_t* addr, int64_t value) {
    if (!NeedSwapMutexes(kRuntimeISA)) {
#if defined(__LP64__)
      *addr = value;
#else
#if defined(__arm__)
#if defined(__ARM_FEATURE_LPAE)
    // If we know that ARM architecture has LPAE (such as Cortex-A15) strd is defined not to tear.
    __asm__ __volatile__("@ QuasiAtomic::Write64\n"
      "strd     %1, %H1, %0"
      : "=m"(*addr)
      : "r" (value));
#else
    // The write is done as a swap so that the cache-line is in the exclusive state for the store.
    int64_t prev;
    int status;
    do {
      __asm__ __volatile__("@ QuasiAtomic::Write64\n"
        "ldrexd     %0, %H0, %2\n"
        "strexd     %1, %3, %H3, %2"
        : "=&r" (prev), "=&r" (status), "+Q"(*addr)
        : "r" (value)
        : "cc");
      } while (UNLIKELY(status != 0));
#endif
#elif defined(__i386__)
      __asm__ __volatile__(
        "movq     %1, %0"
        : "=m" (*addr)
        : "x" (value));
#else
      LOG(FATAL) << "Unsupported architecture";
#endif
#endif  // defined(__LP64__)
    } else {
      SwapMutexWrite64(addr, value);
    }
  }

  // Atomically compare the value at "addr" to "old_value", if equal replace it with "new_value"
  // and return true. Otherwise, don't swap, and return false.
  // This is fully ordered, i.e. it has C++11 memory_order_seq_cst
  // semantics (assuming all other accesses use a mutex if this one does).
  // This has "strong" semantics; if it fails then it is guaranteed that
  // at some point during the execution of Cas64, *addr was not equal to
  // old_value.
  static bool Cas64(int64_t old_value, int64_t new_value, volatile int64_t* addr) {
    if (!NeedSwapMutexes(kRuntimeISA)) {
      return __sync_bool_compare_and_swap(addr, old_value, new_value);
    } else {
      return SwapMutexCas64(old_value, new_value, addr);
    }
  }

  // Does the architecture provide reasonable atomic long operations or do we fall back on mutexes?
  static bool LongAtomicsUseMutexes(InstructionSet isa) {
    return NeedSwapMutexes(isa);
  }

  static void ThreadFenceAcquire() {
    std::atomic_thread_fence(std::memory_order_acquire);
  }

  static void ThreadFenceRelease() {
    std::atomic_thread_fence(std::memory_order_release);
  }

  static void ThreadFenceForConstructor() {
    #if defined(__aarch64__)
      __asm__ __volatile__("dmb ishst" : : : "memory");
    #else
      std::atomic_thread_fence(std::memory_order_release);
    #endif
  }

  static void ThreadFenceSequentiallyConsistent() {
    std::atomic_thread_fence(std::memory_order_seq_cst);
  }

 private:
  static Mutex* GetSwapMutex(const volatile int64_t* addr);
  static int64_t SwapMutexRead64(volatile const int64_t* addr);
  static void SwapMutexWrite64(volatile int64_t* addr, int64_t val);
  static bool SwapMutexCas64(int64_t old_value, int64_t new_value, volatile int64_t* addr);

  // We stripe across a bunch of different mutexes to reduce contention.
  static constexpr size_t kSwapMutexCount = 32;
  static std::vector<Mutex*>* gSwapMutexes;

  DISALLOW_COPY_AND_ASSIGN(QuasiAtomic);
};

template<typename T>
class PACKED(sizeof(T)) Atomic : public std::atomic<T> {
 public:
  Atomic<T>() : std::atomic<T>(T()) { }

  explicit Atomic<T>(T value) : std::atomic<T>(value) { }

  // Load data from an atomic variable with Java data memory order semantics.
  //
  // Promises memory access semantics of ordinary Java data.
  // Does not order other memory accesses.
  // Long and double accesses may be performed 32 bits at a time.
  // There are no "cache coherence" guarantees; e.g. loads from the same location may be reordered.
  // In contrast to normal C++ accesses, racing accesses are allowed.
  T LoadJavaData() const {
    return this->load(std::memory_order_relaxed);
  }

  // Store data in an atomic variable with Java data memory ordering semantics.
  //
  // Promises memory access semantics of ordinary Java data.
  // Does not order other memory accesses.
  // Long and double accesses may be performed 32 bits at a time.
  // There are no "cache coherence" guarantees; e.g. loads from the same location may be reordered.
  // In contrast to normal C++ accesses, racing accesses are allowed.
  void StoreJavaData(T desired_value) {
    this->store(desired_value, std::memory_order_relaxed);
  }

  // Atomically replace the value with desired_value if it matches the expected_value.
  // Participates in total ordering of atomic operations.
  bool CompareAndSetStrongSequentiallyConsistent(T expected_value, T desired_value) {
    return this->compare_exchange_strong(expected_value, desired_value, std::memory_order_seq_cst);
  }

  // The same, except it may fail spuriously.
  bool CompareAndSetWeakSequentiallyConsistent(T expected_value, T desired_value) {
    return this->compare_exchange_weak(expected_value, desired_value, std::memory_order_seq_cst);
  }

  // Atomically replace the value with desired_value if it matches the expected_value. Doesn't
  // imply ordering or synchronization constraints.
  bool CompareAndSetStrongRelaxed(T expected_value, T desired_value) {
    return this->compare_exchange_strong(expected_value, desired_value, std::memory_order_relaxed);
  }

  // Atomically replace the value with desired_value if it matches the expected_value. Prior writes
  // to other memory locations become visible to the threads that do a consume or an acquire on the
  // same location.
  bool CompareAndSetStrongRelease(T expected_value, T desired_value) {
    return this->compare_exchange_strong(expected_value, desired_value, std::memory_order_release);
  }

  // The same, except it may fail spuriously.
  bool CompareAndSetWeakRelaxed(T expected_value, T desired_value) {
    return this->compare_exchange_weak(expected_value, desired_value, std::memory_order_relaxed);
  }

  // Atomically replace the value with desired_value if it matches the expected_value. Prior writes
  // made to other memory locations by the thread that did the release become visible in this
  // thread.
  bool CompareAndSetWeakAcquire(T expected_value, T desired_value) {
    return this->compare_exchange_weak(expected_value, desired_value, std::memory_order_acquire);
  }

  // Atomically replace the value with desired_value if it matches the expected_value. Prior writes
  // to other memory locations become visible to the threads that do a consume or an acquire on the
  // same location.
  bool CompareAndSetWeakRelease(T expected_value, T desired_value) {
    return this->compare_exchange_weak(expected_value, desired_value, std::memory_order_release);
  }

  // Returns the address of the current atomic variable. This is only used by futex() which is
  // declared to take a volatile address (see base/mutex-inl.h).
  volatile T* Address() {
    return reinterpret_cast<T*>(this);
  }

  static T MaxValue() {
    return std::numeric_limits<T>::max();
  }
};

typedef Atomic<int32_t> AtomicInteger;

static_assert(sizeof(AtomicInteger) == sizeof(int32_t), "Weird AtomicInteger size");
static_assert(alignof(AtomicInteger) == alignof(int32_t),
              "AtomicInteger alignment differs from that of underlyingtype");
static_assert(sizeof(Atomic<int64_t>) == sizeof(int64_t), "Weird Atomic<int64> size");

// Assert the alignment of 64-bit integers is 64-bit. This isn't true on certain 32-bit
// architectures (e.g. x86-32) but we know that 64-bit integers here are arranged to be 8-byte
// aligned.
#if defined(__LP64__)
  static_assert(alignof(Atomic<int64_t>) == alignof(int64_t),
                "Atomic<int64> alignment differs from that of underlying type");
#endif

}  // namespace art

#endif  // ART_RUNTIME_ATOMIC_H_
