$xpano_sources = @(Get-ChildItem -Recurse -Path xpano/ -Include *.cc,*.h).fullname
$test_sources = @(Get-ChildItem -Recurse -Path tests/ -Include *.cc,*.h).fullname

clang-format -i @($xpano_sources + $test_sources)
