#!/usr/bin/env bash

usage() {
cat <<END
--------------------------------------------
Little helper script that wraps clang-format
--------------------------------------------
Usage: ./clang-format-helper.bash <option>
  -p | --perform  perform automatic formatting (for developers)
  -t | --test     test that automatic formatting has been performed (for CI builds)
  -h | --help     display this help
END
}

fail() {
    echo "ERROR: $*" 1>&2
    exit 1
}

type clang-format >/dev/null 2>&1 || fail "Clang-format not found"

ver_num=$(clang-format -version)
ver_req='6.0.0'
if [[ "${ver_num:21:5}" != "${ver_req}" ]]; then
    fail "Wrong clang-format version found: ${ver_num:21:5} (${ver_req} is required)"
fi

base_dir=$(dirname $BASH_SOURCE)
FILES_TO_FORMAT="${base_dir}/../src/*.cc ${base_dir}/../src/include/smash/*.h ${base_dir}/../src/tests/*.cc ${base_dir}/../src/tests/*.h"


if [[ $# -ne 1 ]]; then
    fail "Wrong number of arguments. 1 argument required."
fi

case $1 in
    -h | --help )
    usage
    exit 0
    ;;
    -p | --perform )
      echo "Running clang-format ..."
      for i in $FILES_TO_FORMAT;
      do
          clang-format --verbose -i $i;
      done
      ;;
    -t | --test )
      echo "Testing that clang-format does not change the source code in the working directory ..."
      check_flag=0
      for file_under_examination in $FILES_TO_FORMAT; do
	  diff <(cat $file_under_examination) <(clang-format $file_under_examination) > /dev/null;
          if [ $? -ne 0 ]; then
              echo "File $file_under_examination not properly formatted. Comparison with a properly formatted file:";
	      diff <(cat $file_under_examination) <(clang-format $file_under_examination);
	      check_flag=1;
	  fi
      done
      if [ $check_flag -eq 1 ]; then
          fail "Clang-format was not properly run on latest commit."
      else
          echo "PASS: No changes to source code by clang-format."
      fi
      ;;
    * )
      fail "Unrecognised option." ;;
esac
