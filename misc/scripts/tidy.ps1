$tidy_runner = (Get-Command run-clang-tidy).Path
python $tidy_runner xpano tests -p .\build\Release
