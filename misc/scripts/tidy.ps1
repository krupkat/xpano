$xpano_sources = @(Get-ChildItem -Recurse -Path xpano/ -Include *.cc).fullname
$test_sources = @(Get-ChildItem -Recurse -Path tests/ -Include *.cc).fullname

clang-tidy @($xpano_sources + $test_sources) -p .\build\Release
