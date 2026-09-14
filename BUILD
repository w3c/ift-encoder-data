load("@protobuf//bazel:cc_proto_library.bzl", "cc_proto_library")
load("@protobuf//bazel:proto_library.bzl", "proto_library")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")
load("@rules_cc//cc:cc_library.bzl", "cc_library")

filegroup(
    name = "freq_data",
    srcs = glob([
        "data/*.riegeli",
        "data/*.riegeli-*",
        "data/metadata.binpb",
    ]),
    visibility = [
        "//visibility:public",
    ],
)

# The same data as ":freq_data", but filtered down to only the individual code
# point (unigram) counts. See generate_unigrams.cc.
filegroup(
    name = "unigram_freq_data",
    srcs = glob([
        "data/unigram/*.riegeli",
    ]),
    visibility = [
        "//visibility:public",
    ],
)

exports_files([
    "data/metadata.binpb",
] + glob([
    "data/*.riegeli",
    "data/*.riegeli-*",
    "data/unigram/*.riegeli",
]))

proto_library(
    name = "codepoint_count_proto",
    srcs = ["codepoint_count.proto"],
    visibility = [
        "//visibility:public",
    ],
)

cc_proto_library(
    name = "codepoint_count_cc_proto",
    visibility = [
        "//visibility:public",
    ],
    deps = [":codepoint_count_proto"],
)

proto_library(
    name = "metadata_proto",
    srcs = ["metadata.proto"],
    visibility = [
        "//visibility:public",
    ],
)

cc_proto_library(
    name = "metadata_cc_proto",
    visibility = [
        "//visibility:public",
    ],
    deps = [":metadata_proto"],
)

# Shared helpers for locating and reading the riegeli data files.
cc_library(
    name = "data_files",
    srcs = ["data_files.cc"],
    hdrs = ["data_files.h"],
    deps = [
        ":codepoint_count_cc_proto",
        "@abseil-cpp//absl/functional:function_ref",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@riegeli//riegeli/bytes:fd_reader",
        "@riegeli//riegeli/records:record_reader",
    ],
)

cc_binary(
    name = "generate_metadata",
    srcs = ["generate_metadata.cc"],
    deps = [
        ":codepoint_count_cc_proto",
        ":data_files",
        ":metadata_cc_proto",
        "@abseil-cpp//absl/flags:flag",
        "@abseil-cpp//absl/flags:parse",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/log:check",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
    ],
)

cc_binary(
    name = "generate_unigrams",
    srcs = ["generate_unigrams.cc"],
    deps = [
        ":codepoint_count_cc_proto",
        ":data_files",
        "@abseil-cpp//absl/flags:flag",
        "@abseil-cpp//absl/flags:parse",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@riegeli//riegeli/bytes:fd_writer",
        "@riegeli//riegeli/records:record_writer",
    ],
)
