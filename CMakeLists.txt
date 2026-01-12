# https://cliutils.gitlab.io/modern-cmake/README.html
# "This is as low as most projects should go"
# Use PROJECT_IS_TOP_LEVEL if version >= 3.21.
cmake_minimum_required(VERSION 3.15)
project(VADPCM
  VERSION 1.0
  DESCRIPTION "VADPCM audio codec"
  LANGUAGES C)

option(ENABLE_WARNINGS "enable C warning flags")
option(ENABLE_WERROR "treat warnings as errors")
set(SANITIZERS OFF CACHE STRING "enable sanitizers")

add_compile_options(-ffile-prefix-map=${CMAKE_CURRENT_SOURCE_DIR}/=)

if(ENABLE_WARNINGS)
  add_compile_options(
    -Wall -Wextra -Wpointer-arith -Wwrite-strings -Wmissing-prototypes
    -Wdouble-promotion -Winit-self -Wstrict-prototypes -Wno-format-zero-length)
endif()
if(ENABLE_WERROR)
  add_compile_options(-Werror)
endif()

if(SANITIZERS)
  add_compile_options("-fsanitize=${SANITIZERS}")
  add_link_options("-fsanitize=${SANITIZERS}")
endif()

add_compile_options(-pthread)
add_link_options(-pthread)

include_directories(.)

add_library(vadpcm STATIC
  codec/autocorr.c
  codec/decode.c
  codec/encode.c
  codec/error.c
  codec/predictor.c
  codec/random.c
)

find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
  target_link_libraries(vadpcm PUBLIC ${MATH_LIBRARY})
endif()

add_library(common STATIC
  common/aiff_common.c
  common/aiff_parse.c
  common/aiff_write.c
  common/alloc.c
  common/audio_read_pcm.c
  common/audio_read_vadpcm.c
  common/binary.c
  common/extended.c
  common/extension.c
  common/file.c
  common/log.c
  common/util.c
)

if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

  # Build and run tests.
  include(CTest)
  if(BUILD_TESTING)
    add_executable(codec_test
      tests/decode_test.c
      tests/encode_test.c
      tests/extension_test.c
      tests/predictor_test.c
      tests/test.c
    )
    target_link_libraries(codec_test common vadpcm ${MATH_LIBRARY})
    add_test(
      NAME codec_test
      COMMAND $<TARGET_FILE:codec_test>
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    )
  endif()

  # Export compilation database to correct location.
  if(UNIX)
    execute_process(
      COMMAND ln -sf
      ${CMAKE_BINARY_DIR}/compile_commands.json
      ${CMAKE_CURRENT_SOURCE_DIR}/compile_commands.json)
  endif()

endif()

add_executable(vadpcm_cli
  vadpcm/cmd_decode.c
  vadpcm/cmd_encode.c
  vadpcm/vadpcm.c
)
set_target_properties(vadpcm_cli PROPERTIES OUTPUT_NAME vadpcm)
target_link_libraries(vadpcm_cli common vadpcm ${MATH_LIBRARY})

add_executable(vadpcmstats
  vadpcmstats/vadpcmstats.c
)
target_link_libraries(vadpcmstats common vadpcm ${MATH_LIBRARY})
