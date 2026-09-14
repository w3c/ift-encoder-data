#ifndef DATA_FILES_H_
#define DATA_FILES_H_

#include <map>
#include <string>
#include <vector>

#include "absl/functional/function_ref.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"
#include "codepoint_count.pb.h"

namespace ift_encoder_data {

// Maps the logical name of a data file (eg. "Language_ja.riegeli") to the
// sorted list of physical files which hold it. A file which isn't sharded has
// exactly one physical path.
using DataFileMap = std::map<std::string, std::vector<std::string>>;

// Finds all of the riegeli data files in `directory` and groups them by logical
// name.
//
// Shards (`name.riegeli-XXXXX-of-YYYYY`) are grouped under the logical name
// (`name.riegeli`) and the physical paths of each logical file are sorted.
// Sub directories and files which aren't riegeli data files are ignored.
absl::StatusOr<DataFileMap> FindDataFiles(absl::string_view directory);

// Returns the name used to refer to a logical file, which for sharded files is
// the logical name with the `@*` suffix (eg. "Language_ja.riegeli@*").
std::string LogicalFileName(const std::string& logical_name,
                            const std::vector<std::string>& physical_paths);

// Reads all of the records in `paths` (the physical files of one logical file,
// in order) and invokes `callback` once per record.
absl::Status ForEachRecord(
    const std::vector<std::string>& paths,
    absl::FunctionRef<void(const CodepointCount&)> callback);

}  // namespace ift_encoder_data

#endif  // DATA_FILES_H_
