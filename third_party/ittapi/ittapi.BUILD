# Copyright 2026 The OpenXLA Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ==============================================================================

# Description:
#   Intel ITT API static stub. It forwards calls to the collector library named
#   by INTEL_LIBITTNOTIFY64 (e.g. VTune) and is a no-op without one.

load("@rules_cc//cc:cc_library.bzl", "cc_library")

licenses(["notice"])  # BSD-3-Clause

exports_files(["LICENSES/BSD-3-Clause.txt"])

cc_library(
    name = "ittnotify",
    srcs = [
        "src/ittnotify/disable_warnings.h",
        "src/ittnotify/ittnotify_config.h",
        "src/ittnotify/ittnotify_static.c",
        "src/ittnotify/ittnotify_static.h",
        "src/ittnotify/ittnotify_types.h",
    ],
    hdrs = [
        "include/ittnotify.h",
        "include/legacy/ittnotify.h",
    ],
    # A private symbol prefix keeps this copy from clashing with other static
    # ITT copies linked into the same binary (e.g. the one bundled in oneDNN).
    defines = ["INTEL_ITTNOTIFY_PREFIX=__xla_itt_"],
    includes = ["include"],
    linkopts = [
        "-ldl",
        "-lpthread",
    ],
    visibility = ["//visibility:public"],
)
