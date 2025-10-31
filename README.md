# Unicode code point frequency data

Data Version: 1.0

## Overview

This is a collection of Unicode code point frequency data gathered from across the web. Frequencies
are provided for individual code points and code point pairs, where each frequency count is the number
of web pages that particular code point or pair is found on.

Note: the code points do not need to occur next to each other in the source page to be counted.
See the collection methodology section for more information.

The actual data files are hosted on a CDN under `https://www.gstatic.com/fonts/unicode_frequency/v1/`.
The list of data files that are present is given by [DATA_FILE_LIST](https://www.gstatic.com/fonts/unicode_frequency/v1/DATA_FILE_LIST). Some of the
larger files are split into multiple shards, these will have a suffix of the form:
`filename.riegeli-*-of-*`.

The frequency data files listed in [DATA_FILE_LIST](https://www.gstatic.com/fonts/unicode_frequency/v1/DATA_FILE_LIST) are released under the
[W3C Software and Document License](https://www.w3.org/copyright/software-license-2023/).
See [LICENSE](https://www.gstatic.com/fonts/unicode_frequency/v1/LICENSE)

A bash script is provided in this repository to automate downloading of all the data files:

```
$ ./data/download-freq-data.sh
```

## Schema

The frequency data files are encoded with [Riegeli](https://github.com/google/riegeli). Each record
is a serialized protobuf with the following schema: [unicode_count.proto](https://www.gstatic.com/fonts/unicode_frequency/v1/unicode_count.proto)

This data set contains the frequencies for pairs of code points. Each record will have exactly two
`codepoints` fields. Records that list the same code point twice give the frequency of that code point
individually.

Frequency data is collected by both language and script. The file name will be either:

* `Language_<language code>.riegeli`, where `<language code>` is a bcp 47 tag, or
* `Script_<script name>.csv`.

## Tools

The [ift-encoder](https://github.com/w3c/ift-encoder) library provides tools and libraries for interacting
with these data files:

* [freq_data_to_sorted_code points](https://github.com/w3c/ift-encoder/blob/main/util/freq_data_to_sorted_codepoints.cc):
  can pull out single code point frequencies and output them in a text format. Example usage:
  
  `bazel run util:freq_data_to_sorted_codepoints -- "Language_ja.riegeli@*" --add_character > japanese-freqs.txt`

* [ift-encoder util::LoadFrequenciesFromRiegeli](https://github.com/w3c/ift-encoder/blob/main/util/load_codepoints.h):
  provides a C++ API for loading these files.

These also provide a demonstration for how to use the Riegeli library to parse the files. Both of these are capable of
handling sharded data files. When loading a file that is sharded append `@*` to the file name. For example
`Language_ja.riegeli@*`.

## Collection Methodology

* Pages from a web search index are first randomly sampled.
  * Note: this means that reported counts are not absolute and should be interpreted relatively
    within a particular file.
* Each selected page is analyzed to determine the language that it is written in. Pages with a low
  confidence language detection are discarded.
* Based on the detected language an associated writing script is selected.
  * Note: this means for some scripts counts are influenced by page samples from multiple languages,
    the most prominent example of this is latin which includes many languages.
* For each unique code point pair on a page the associated count for that script and language is incremented by 1.
  * Here a code point pair just means that both code points are present somewhere on the page, it
    does not require they occur next to each other in the text.
  * Each unique pair is counted only once per page.
* Within a script code points are filtered to those used in that script, using the definitions in
  [googlefonts/nam-files](https://github.com/googlefonts/nam-files/tree/main/Lib/gfsubsets/data)
* In addition to the individual CJK scripts an overall CJK code point frequency count is collected by
  combining all of the Chinese, Japanese, and Korean counts. This can be found in `Script_CJK.riegeli`.
* `Script_emoji.csv` and `Script_symbols.csv` are based on counts across all scripts.
* `fallback.csv` collects up counts across all scripts of any code points which are not
  associated with any other script.
