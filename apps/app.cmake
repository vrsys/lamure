macro(InitApp APP_NAME)

  cmake_minimum_required(VERSION 2.8.7)
  project(${APP_NAME}_app)

  file(GLOB HEADER_FILES ${PROJECT_SOURCE_DIR}/*.h)
  file(GLOB SRC_FILES ${PROJECT_SOURCE_DIR}/*.cpp)

  add_executable(${PROJECT_NAME} ${SRC_FILES} ${HEADER_FILES})
  set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${APP_NAME})

  if(MSVC)
    install(TARGETS ${PROJECT_NAME}
      CONFIGURATIONS Release
      RUNTIME DESTINATION bin/Release
      LIBRARY DESTINATION lib/Release
      ARCHIVE DESTINATION lib/Release
      )
    install(TARGETS ${PROJECT_NAME}
      CONFIGURATIONS Debug
      RUNTIME DESTINATION bin/Debug
      LIBRARY DESTINATION lib/Debug
      ARCHIVE DESTINATION lib/Debug
      )
  elseif(UNIX)
    install(TARGETS ${PROJECT_NAME}
      RUNTIME DESTINATION bin
      LIBRARY DESTINATION lib
      ARCHIVE DESTINATION lib
      )

      file(GLOB SCRIPT_FILES . ${PROJECT_SOURCE_DIR}/*.sh)

      install(PROGRAMS ${SCRIPT_FILES} DESTINATION bin)
  endif(MSVC)
endmacro()

macro(MsvcPostBuild TARGET_NAME)
  if(MSVC)
    ADD_CUSTOM_COMMAND ( TARGET ${TARGET_NAME} POST_BUILD 
      COMMAND robocopy \"${LIBRARY_OUTPUT_PATH}/$(Configuration)/\" \"${EXECUTABLE_OUTPUT_PATH}/$(Configuration)/\" *.dll /R:0 /W:0 /NP 
      & robocopy \"${GLOBAL_EXT_DIR}/\" \"${EXECUTABLE_OUTPUT_PATH}/$(Configuration)/\" *.dll /R:0 /W:0 /NP 
      & robocopy \"${GLOBAL_EXT_DIR}/\" \"${EXECUTABLE_OUTPUT_PATH}/$(Configuration)/\" *.exe /R:0 /W:0 /NP \n if %ERRORLEVEL% LEQ 7 (exit /b 0) else (exit /b 1)
      )
  endif(MSVC)
endmacro()

