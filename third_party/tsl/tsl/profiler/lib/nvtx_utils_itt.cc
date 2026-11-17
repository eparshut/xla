/* Copyright 2026 The OpenXLA Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

// Intel ITT API implementation of the profiler range interface, used when XLA
// is built for SYCL. Ranges become ITT tasks in the "TSL" domain, so they show
// up in Intel VTune Profiler (or any ITT collector) the same way NVTX ranges
// show up in Nsight Systems.

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "absl/base/no_destructor.h"
#include "absl/container/flat_hash_map.h"
#include "absl/strings/string_view.h"
#include "absl/synchronization/mutex.h"
#include "ittnotify.h"
#include "tsl/profiler/lib/nvtx_utils.h"

namespace tsl::profiler {

// The ITT handle is created on first use as a task name, so large registered
// strings that are never used as names do not reach the collector.
struct String {
  explicit String(absl::string_view text) : text(text) {}

  const std::string text;
  std::atomic<__itt_string_handle*> handle = nullptr;
};

namespace {

__itt_domain* ToItt(ProfilerDomainHandle domain) {
  return reinterpret_cast<__itt_domain*>(domain);
}

// Like NVTX registrations, interned strings live until process exit.
String* Intern(absl::string_view text) {
  static absl::NoDestructor<absl::Mutex> mutex;
  static absl::NoDestructor<
      absl::flat_hash_map<std::string, std::unique_ptr<String>>>
      strings;
  {
    absl::ReaderMutexLock lock(*mutex);
    if (auto it = strings->find(text); it != strings->end()) {
      return it->second.get();
    }
  }
  absl::MutexLock lock(*mutex);
  std::unique_ptr<String>& string = (*strings)[text];
  if (string == nullptr) {
    string = std::make_unique<String>(text);
  }
  return string.get();
}

__itt_string_handle* ToItt(String* string) {
  if (string == nullptr) {
    return nullptr;
  }
  __itt_string_handle* handle = string->handle.load(std::memory_order_acquire);
  if (handle == nullptr) {
    // ITT de-duplicates handles by content, so a racing create is harmless.
    handle = __itt_string_handle_create(string->text.c_str());
    string->handle.store(handle, std::memory_order_release);
  }
  return handle;
}

}  // namespace

ProfilerDomainHandle DefaultProfilerDomain() {
  // Null unless an ITT collector is attached, which keeps all annotation work
  // disabled otherwise.
  static __itt_domain* const domain = __itt_domain_create("TSL");
  return reinterpret_cast<ProfilerDomainHandle>(domain);
}

void NameCurrentThread(const std::string& thread_name) {
  __itt_thread_set_name(thread_name.c_str());
}

// ITT has no device or stream naming; collectors name GPU queues themselves.
void NameDevice(int, const std::string&) {}
void NameStream(StreamHandle, const std::string&) {}

StringHandle RegisterString(ProfilerDomainHandle, const std::string& str) {
  return Intern(str);
}

void RangePush(ProfilerDomainHandle domain, const char* ascii) {
  __itt_task_begin(ToItt(domain), __itt_null, __itt_null,
                   ToItt(Intern(ascii)));
}

void RangePop(ProfilerDomainHandle domain) { __itt_task_end(ToItt(domain)); }

namespace detail {
void RangePush(ProfilerDomainHandle domain, StringHandle title, uint64_t,
               const void*, size_t) {
  __itt_task_begin(ToItt(domain), __itt_null, __itt_null, ToItt(title));
}
}  // namespace detail

uint64_t RegisterSchema(ProfilerDomainHandle, const void*) { return 0; }

void MarkMemoryInitialized(void const*, size_t, StreamHandle) {}

}  // namespace tsl::profiler
