$xpano_sources = @(Get-ChildItem -Recurse -Path xpano/ -Include *.cc,*.h).fullname
$test_sources = @(Get-ChildItem -Recurse -Path tests/ -Include *.cc,*.h).fullname

clang-include-cleaner -p build --edit --insert --remove --ignore-headers="SDL.*,opencv2.*,exiv2.*,spdlog\\fmt\\.*" @($xpano_sources + $test_sources)
