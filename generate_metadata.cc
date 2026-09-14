#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "codepoint_count.pb.h"
#include "data_files.h"
#include "metadata.pb.h"

ABSL_FLAG(std::string, input_dir, "data", "Directory containing riegeli files.");
ABSL_FLAG(std::string, output_file, "metadata.binpb",
          "Output path for the metadata proto file.");

using ift_encoder_data::CodepointCount;
using ift_encoder_data::DataFileMap;
using ift_encoder_data::DatasetMetadata;
using ift_encoder_data::FileMetadata;
using ift_encoder_data::FindDataFiles;
using ift_encoder_data::ForEachRecord;
using ift_encoder_data::LogicalFileName;

absl::StatusOr<std::set<uint32_t>> GetCodepointsFromFiles(
    const std::vector<std::string>& paths) {
  std::set<uint32_t> codepoints;
  absl::Status status =
      ForEachRecord(paths, [&codepoints](const CodepointCount& record) {
        codepoints.insert(record.codepoints().begin(),
                          record.codepoints().end());
      });
  if (!status.ok()) {
    return status;
  }
  return codepoints;
}

absl::Status ProcessDataset() {
  std::string input_dir = absl::GetFlag(FLAGS_input_dir);
  std::string output_file = absl::GetFlag(FLAGS_output_file);

  auto data_files = FindDataFiles(input_dir);
  if (!data_files.ok()) {
    return data_files.status();
  }

  absl::Status status;

  DatasetMetadata dataset_metadata;
  for (const auto& [logical_name, physical_paths] : *data_files) {
    auto codepoints_or = GetCodepointsFromFiles(physical_paths);
    if (!codepoints_or.ok()) {
      LOG(ERROR) << "Failed to process " << logical_name << ": "
                 << codepoints_or.status();
      status.Update(codepoints_or.status());
      continue;
    }

    FileMetadata* file_metadata = dataset_metadata.add_files();
    // Use the @* notation for sharded files in the metadata as well, for consistency.
    file_metadata->set_file_name(
        LogicalFileName(logical_name, physical_paths));

    for (uint32_t cp : *codepoints_or) {
      file_metadata->add_codepoints(cp);
    }
  }

  if (!status.ok()) {
    return status;
  }

  // Write out as a standard binary protobuf.
  std::ofstream out(output_file, std::ios::binary);
  if (!dataset_metadata.SerializeToOstream(&out)) {
    return absl::InternalError("Failed to serialize metadata to binary proto.");
  }

  LOG(INFO) << "Metadata written to " << output_file;
  return absl::OkStatus();
}

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  absl::Status status = ProcessDataset();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to generate metadata: " << status;
    return 1;
  }
  return 0;
}
