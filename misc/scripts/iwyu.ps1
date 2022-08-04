$cwd = (Get-Location).Path
$iwyu_tool = (Get-Command iwyu_tool.py).Path

$extra_args = "--driver-mode=cl", "-Xiwyu", "--no_fwd_decls",
  "-Xiwyu", "--mapping_file=${cwd}\misc\mappings\iwyu_win.imp"

python $iwyu_tool xpano -p .\build\Release -- @extra_args
python $iwyu_tool tests -p .\build\Release -- @extra_args
