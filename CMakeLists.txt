cmake_minimum_required (VERSION 3.8)

# MSVC Hot Reload
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()


project ("TankiCodeBackupCpp")

# Enable C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Libs
include(FetchContent)
FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
                         GIT_TAG 1.11.2)
set(CPR_BUILD_TESTS OFF)
set(CPR_FORCE_USE_SYSTEM_CURL OFF)
set(BUILD_SHARED_LIBS OFF)
FetchContent_MakeAvailable(cpr)

# Mac: Compile with llvm and link libidn2
if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    message(STATUS "Using Homebrew LLVM toolchain on macOS")

    set(HOMEBREW_LLVM_ROOT "/opt/homebrew/opt/llvm")

    set(CMAKE_C_COMPILER "${HOMEBREW_LLVM_ROOT}/bin/clang" CACHE STRING "" FORCE)
    set(CMAKE_CXX_COMPILER "${HOMEBREW_LLVM_ROOT}/bin/clang++" CACHE STRING "" FORCE)

    include_directories("${HOMEBREW_LLVM_ROOT}/include")
    link_directories("${HOMEBREW_LLVM_ROOT}/lib/c++")

    add_compile_options(-fexperimental-library -Wno-deprecated-declarations)
    add_link_options(-L"${HOMEBREW_LLVM_ROOT}/lib/c++" -Wl,-rpath,"${HOMEBREW_LLVM_ROOT}/lib/c++")

    set(IDN2_ROOT "/opt/homebrew/opt/libidn2")
    include_directories("${IDN2_ROOT}/include")
    link_directories("${IDN2_ROOT}/lib")
endif()

# Executables
add_executable (TankiCodeBackupCpp "TankiCodeBackupCpp.cpp")

# Link
target_link_libraries(TankiCodeBackupCpp PRIVATE cpr::cpr)

# Compress
find_program(UPX_EXECUTABLE upx HINTS ${CMAKE_SOURCE_DIR})
if(CMAKE_BUILD_TYPE STREQUAL "MinSizeRel" AND UPX_EXECUTABLE)
    add_custom_command(TARGET TankiCodeBackupCpp POST_BUILD
        COMMAND ${UPX_EXECUTABLE} --best --lzma "$<TARGET_FILE:TankiCodeBackupCpp>"
        COMMENT "Compressing executable with UPX..."
    )
endif()
