#include "data_files.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include "absl/functional/function_ref.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "codepoint_count.pb.h"
#include "riegeli/bytes/fd_reader.h"
#include "riegeli/records/record_reader.h"

namespace ift_encoder_data {

absl::StatusOr<DataFileMap> FindDataFiles(absl::string_view directory) {
  std::filesystem::path dir(directory);
  std::error_code error_code;
  std::filesystem::directory_iterator it(dir, error_code);
  if (error_code) {
    return absl::NotFoundError(absl::StrCat("Failed to list ", directory, ": ",
                                            error_code.message()));
  }

  DataFileMap data_files;
  for (const auto& entry : it) {
    if (!entry.is_regular_file()) continue;

    std::string filename = entry.path().filename().string();
    std::string full_path = entry.path().string();

    if (absl::EndsWith(filename, ".riegeli")) {
      data_files[filename].push_back(full_path);
      continue;
    }

    // Check for shards: name.riegeli-XXXXX-of-YYYYY
    size_t riegeli_pos = filename.find(".riegeli-");
    if (riegeli_pos != std::string::npos) {
      std::string logical_name = filename.substr(0, riegeli_pos) + ".riegeli";
      data_files[logical_name].push_back(full_path);
    }
  }

  for (auto& [logical_name, physical_paths] : data_files) {
    std::sort(physical_paths.begin(), physical_paths.end());
  }

  return data_files;
}

std::string LogicalFileName(const std::string& logical_name,
                            const std::vector<std::string>& physical_paths) {
  if (physical_paths.size() > 1) {
    return absl::StrCat(logical_name, "@*");
  }
  return logical_name;
}

absl::Status ForEachRecord(
    const std::vector<std::string>& paths,
    absl::FunctionRef<void(const CodepointCount&)> callback) {
  for (const std::string& path : paths) {
    LOG(INFO) << "Processing " << path;
    riegeli::RecordReader reader((riegeli::FdReader(path)));
    if (!reader.ok()) {
      return reader.status();
    }

    CodepointCount record;
    while (reader.ReadRecord(record)) {
      callback(record);
    }

    if (!reader.Close()) {
      return reader.status();
    }
  }
  return absl::OkStatus();
}

}  // namespace ift_encoder_data
