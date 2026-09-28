# Set folder for external projects (for project tree in IDE)
set(CMAKE_FOLDER externals)

option(VITA3K_ENABLE_TRACY "Enable Tracy profiler integration (off by default for release performance)" OFF)

option(BUILD_SHARED_LIBS "Build shared library" OFF)
option(CAPSTONE_ARCHITECTURE_DEFAULT "Whether architectures are enabled by default" OFF)
option(CAPSTONE_ARM_SUPPORT "ARM support" ON)
add_subdirectory(capstone EXCLUDE_FROM_ALL)
