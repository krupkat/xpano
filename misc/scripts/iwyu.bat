for /F %%i in ('which iwyu_tool.py') do python %%i xpano -p .\build\Release -- --driver-mode=cl -Xiwyu --mapping_file=%cd%\misc\mappings\iwyu_win.imp -Xiwyu --no_fwd_decls
