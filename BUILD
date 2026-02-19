load("@protobuf//bazel:cc_proto_library.bzl", "cc_proto_library")
load("@protobuf//bazel:proto_library.bzl", "proto_library")
load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

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

proto_library(
    name = "codepoint_count_proto",
    srcs = ["codepoint_count.proto"],
)

cc_proto_library(
    name = "codepoint_count_cc_proto",
    deps = [":codepoint_count_proto"],
)

proto_library(
    name = "metadata_proto",
    srcs = ["metadata.proto"],
)

cc_proto_library(
    name = "metadata_cc_proto",
    deps = [":metadata_proto"],
)

cc_binary(
    name = "generate_metadata",
    srcs = ["generate_metadata.cc"],
    deps = [
        ":codepoint_count_cc_proto",
        ":metadata_cc_proto",
        "@abseil-cpp//absl/flags:flag",
        "@abseil-cpp//absl/flags:parse",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/log:check",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@riegeli//riegeli/bytes:fd_reader",
        "@riegeli//riegeli/records:record_reader",
    ],
)
