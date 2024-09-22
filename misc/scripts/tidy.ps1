$tidy_runner = (Get-Command run-clang-tidy).Path
python $tidy_runner xpano\\xpano xpano\\tests -p .\build -quiet
