cmake_minimum_required(VERSION 3.6)

project(BOREALIS_DEMO)

# armeabi-v7a: import cpufeatures only when the legacy NDK module is available
if(ANDROID_ABI MATCHES "armeabi-v7a")
    include(AndroidNdkModules OPTIONAL)
    if(COMMAND android_ndk_import_module_cpufeatures)
        android_ndk_import_module_cpufeatures()
    endif()
endif()


# SDL sources are in a subfolder named "SDL"
add_subdirectory(SDL)

# Compilation of companion libraries
#add_subdirectory(SDL_image)
#add_subdirectory(SDL_mixer)
#add_subdirectory(SDL_ttf)

# Your game and its CMakeLists.txt are in a subfolder named "src"
#add_subdirectory(src)


set(PLATFORM_ANDROID ON CACHE BOOL "")

add_subdirectory(borealis)
