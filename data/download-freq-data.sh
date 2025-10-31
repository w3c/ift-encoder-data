#!/bin/bash

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

wget "https://www.gstatic.com/fonts/unicode_frequency/v1/DATA_FILE_LIST" -O $SCRIPT_DIR/DATA_FILE_LIST
for f in $(cat $SCRIPT_DIR/DATA_FILE_LIST); do
  wget "https://www.gstatic.com/fonts/unicode_frequency/v1/$f" -O $SCRIPT_DIR/$f
done
