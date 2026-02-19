#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/check.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "codepoint_count.pb.h"
#include "metadata.pb.h"
#include "riegeli/bytes/fd_reader.h"
#include "riegeli/records/record_reader.h"

ABSL_FLAG(std::string, input_dir, "data", "Directory containing riegeli files.");
ABSL_FLAG(std::string, output_file, "metadata.binpb",
          "Output path for the metadata proto file.");

using ift_encoder_data::CodepointCount;
using ift_encoder_data::DatasetMetadata;
using ift_encoder_data::FileMetadata;

absl::StatusOr<std::set<uint32_t>> GetCodepointsFromFiles(
    const std::vector<std::string>& paths) {
  std::set<uint32_t> codepoints;
  for (const std::string& path : paths) {
    LOG(INFO) << "Processing " << path;
    riegeli::RecordReader reader((riegeli::FdReader(path)));
    if (!reader.ok()) {
      return reader.status();
    }

    CodepointCount record;
    while (reader.ReadRecord(record)) {
      for (uint32_t cp : record.codepoints()) {
        codepoints.insert(cp);
      }
    }

    if (!reader.Close()) {
      return reader.status();
    }
  }
  return codepoints;
}

absl::Status ProcessDataset() {
  std::string input_dir = absl::GetFlag(FLAGS_input_dir);
  std::string output_file = absl::GetFlag(FLAGS_output_file);

  std::map<std::string, std::vector<std::string>> logical_to_physical;
  for (const auto& entry : std::filesystem::directory_iterator(input_dir)) {
    if (!entry.is_regular_file()) continue;

    std::string filename = entry.path().filename().string();
    std::string full_path = entry.path().string();

    if (absl::EndsWith(filename, ".riegeli")) {
      logical_to_physical[filename].push_back(full_path);
    } else {
      // Check for shards: name.riegeli-XXXXX-of-YYYYY
      size_t riegeli_pos = filename.find(".riegeli-");
      if (riegeli_pos != std::string::npos) {
        std::string logical_name = filename.substr(0, riegeli_pos) + ".riegeli";
        logical_to_physical[logical_name].push_back(full_path);
      }
    }
  }

  absl::Status status;

  DatasetMetadata dataset_metadata;
  for (auto& [logical_name, physical_paths] : logical_to_physical) {
    std::sort(physical_paths.begin(), physical_paths.end());
    auto codepoints_or = GetCodepointsFromFiles(physical_paths);
    if (!codepoints_or.ok()) {
      LOG(ERROR) << "Failed to process " << logical_name << ": "
                 << codepoints_or.status();
      status.Update(codepoints_or.status());
      continue;
    }

    FileMetadata* file_metadata = dataset_metadata.add_files();
    // Use the @* notation for sharded files in the metadata as well, for consistency.
    if (physical_paths.size() > 1) {
      file_metadata->set_file_name(absl::StrCat(logical_name, "@*"));
    } else {
      file_metadata->set_file_name(logical_name);
    }

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
