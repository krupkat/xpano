function(copy_runtime_dlls target)
  set(copy_command "copy_if_different;$<TARGET_RUNTIME_DLLS:${target}>;$<TARGET_FILE_DIR:${target}>")
  add_custom_command(TARGET ${target} PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E "$<IF:$<BOOL:$<TARGET_RUNTIME_DLLS:${target}>>,${copy_command},true>"
    COMMAND_EXPAND_LISTS
  )
endfunction()

function(copy_directory target directory)
  cmake_path(GET directory FILENAME dir_name)
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${directory} $<TARGET_FILE_DIR:${target}>/${dir_name}
    COMMAND_EXPAND_LISTS
  )
endfunction()
