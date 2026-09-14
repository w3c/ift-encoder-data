#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <map>
#include <set>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/log/log.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"
#include "codepoint_count.pb.h"
#include "data_files.h"
#include "riegeli/bytes/fd_writer.h"
#include "riegeli/records/record_writer.h"

ABSL_FLAG(std::string, input_dir, "data",
          "Directory containing the (bigram) riegeli files.");
ABSL_FLAG(std::string, output_dir, "data/unigram",
          "Directory to write the unigram only riegeli files into.");

using ift_encoder_data::CodepointCount;
using ift_encoder_data::FindDataFiles;
using ift_encoder_data::ForEachRecord;

// Maps a code point to the number of pages it was seen on.
using UnigramCounts = std::map<uint32_t, uint64_t>;

// Reads all of the (possibly sharded) physical files which make up one logical
// data file and returns the counts of only the unigram records found in them.
//
// In this data set a unigram (a count for an individual code point) is encoded
// as a record which lists the same code point twice. Records which reference
// two distinct code points are bigrams and are dropped.
//
// If `bigram_only_codepoints` is not null it is populated with the code points
// which are present in the input but have no unigram record of their own, that
// is, the code points whose coverage is lost by this filtering.
absl::StatusOr<UnigramCounts> GetUnigramsFromFiles(
    const std::vector<std::string>& paths,
    std::set<uint32_t>* bigram_only_codepoints) {
  UnigramCounts unigrams;
  std::set<uint32_t> all_codepoints;

  absl::Status status = ForEachRecord(
      paths, [&unigrams, &all_codepoints](const CodepointCount& record) {
        if (record.codepoints().empty()) {
          return;
        }

        all_codepoints.insert(record.codepoints().begin(),
                              record.codepoints().end());

        // A record is a unigram if it references exactly one distinct code
        // point (either listed once, or, as this data set does, listed twice).
        uint32_t codepoint = record.codepoints(0);
        bool is_unigram =
            std::all_of(record.codepoints().begin(), record.codepoints().end(),
                        [codepoint](uint32_t cp) { return cp == codepoint; });
        if (!is_unigram) {
          return;
        }

        // Counts are summed in case the same code point shows up in more than
        // one shard of the same logical file.
        unigrams[codepoint] += record.count();
      });
  if (!status.ok()) {
    return status;
  }

  if (bigram_only_codepoints != nullptr) {
    for (uint32_t codepoint : all_codepoints) {
      if (unigrams.find(codepoint) == unigrams.end()) {
        bigram_only_codepoints->insert(codepoint);
      }
    }
  }
  return unigrams;
}

// Writes the unigram counts to a single, unsharded, riegeli file. Records are
// written ordered by descending count, matching the ordering of the input data
// files, and keep the two identical code points encoding used by the input
// files so that existing readers work unchanged.
absl::Status WriteUnigrams(const UnigramCounts& unigrams,
                           const std::string& path) {
  // Code points with equal counts are ordered by code point to keep the output
  // deterministic.
  std::vector<std::pair<uint32_t, uint64_t>> by_count(unigrams.begin(),
                                                      unigrams.end());
  std::sort(by_count.begin(), by_count.end(),
            [](const std::pair<uint32_t, uint64_t>& a,
               const std::pair<uint32_t, uint64_t>& b) {
              if (a.second != b.second) {
                return a.second > b.second;
              }
              return a.first < b.first;
            });

  riegeli::RecordWriter writer(
      riegeli::FdWriter<>(path),
      riegeli::RecordWriterBase::Options().set_transpose(true));
  if (!writer.ok()) {
    return writer.status();
  }

  CodepointCount record;
  for (const auto& [codepoint, count] : by_count) {
    record.Clear();
    record.add_codepoints(codepoint);
    record.add_codepoints(codepoint);
    record.set_count(count);
    if (!writer.WriteRecord(record)) {
      return writer.status();
    }
  }

  if (!writer.Close()) {
    return writer.status();
  }
  return absl::OkStatus();
}

absl::Status ProcessDataset() {
  std::string input_dir = absl::GetFlag(FLAGS_input_dir);
  std::string output_dir = absl::GetFlag(FLAGS_output_dir);

  auto data_files = FindDataFiles(input_dir);
  if (!data_files.ok()) {
    return data_files.status();
  }

  std::error_code error_code;
  std::filesystem::create_directories(output_dir, error_code);
  if (error_code) {
    return absl::InternalError(absl::StrCat("Failed to create output dir ",
                                            output_dir, ": ",
                                            error_code.message()));
  }

  absl::Status status;

  for (const auto& [logical_name, physical_paths] : *data_files) {
    std::set<uint32_t> bigram_only_codepoints;
    auto unigrams = GetUnigramsFromFiles(physical_paths,
                                         &bigram_only_codepoints);
    if (!unigrams.ok()) {
      LOG(ERROR) << "Failed to read " << logical_name << ": "
                 << unigrams.status();
      status.Update(unigrams.status());
      continue;
    }

    // The unigram data is small enough that it's always written as a single
    // unsharded file, even if the input was sharded.
    std::string output_path =
        (std::filesystem::path(output_dir) / logical_name).string();
    absl::Status write_status = WriteUnigrams(*unigrams, output_path);
    if (!write_status.ok()) {
      LOG(ERROR) << "Failed to write " << output_path << ": " << write_status;
      status.Update(write_status);
      continue;
    }
    LOG(INFO) << "Wrote " << unigrams->size() << " unigrams to " << output_path;
    if (!bigram_only_codepoints.empty()) {
      // Not an error, just a heads up that the unigram file covers fewer code
      // points than the input file it was derived from.
      LOG(WARNING) << logical_name << ": " << bigram_only_codepoints.size()
                   << " code point(s) dropped, they only appear in bigram "
                      "records and have no count of their own.";
    }
  }

  if (!status.ok()) {
    return status;
  }

  return absl::OkStatus();
}

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  absl::Status status = ProcessDataset();
  if (!status.ok()) {
    LOG(ERROR) << "Failed to generate unigram data: " << status;
    return 1;
  }
  return 0;
}
