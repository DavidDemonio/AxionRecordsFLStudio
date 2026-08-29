cmake_minimum_required(VERSION 3.25)

project(SteamMicSend VERSION 1.0.0 LANGUAGES C CXX)

include(FetchContent)

set(SMTG_ENABLE_VSTGUI_SUPPORT OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VST3_PLUGIN_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_ENABLE_VST3_HOSTING_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SMTG_RUN_VST_VALIDATOR OFF CACHE BOOL "" FORCE)
set(SMTG_CREATE_PLUGIN_LINK OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    vst3sdk
    GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
    GIT_TAG master
    GIT_SHALLOW FALSE
)
FetchContent_MakeAvailable(vst3sdk)
smtg_enable_vst3_sdk()

FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 0.11.23
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(miniaudio)

set(STEAMMIC_SOURCES
    ${vst3sdk_SOURCE_DIR}/public.sdk/source/vst/vstsinglecomponenteffect.cpp
    Source/SteamMicSend.h
    Source/SteamMicSend.cpp
    Source/entry.cpp
)

smtg_add_vst3plugin(SteamMicSend ${STEAMMIC_SOURCES})

target_compile_features(SteamMicSend PRIVATE cxx_std_17)
target_include_directories(SteamMicSend PRIVATE ${miniaudio_SOURCE_DIR})
target_link_libraries(SteamMicSend PRIVATE sdk)

target_compile_definitions(SteamMicSend PRIVATE
    NOMINMAX
    WIN32_LEAN_AND_MEAN
)

if(MSVC)
    target_compile_options(SteamMicSend PRIVATE /W4 /permissive- /EHsc)
endif()
