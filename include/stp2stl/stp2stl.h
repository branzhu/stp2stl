#pragma once

#if !defined(STP2STL_PLATFORM_WINDOWS)
  #if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(_WINDOWS) || defined(__WIN32__) || defined(__WINDOWS__) || defined(_MSC_VER)
    #define STP2STL_PLATFORM_WINDOWS 1
  #else
    #define STP2STL_PLATFORM_WINDOWS 0
  #endif
#endif

#if STP2STL_PLATFORM_WINDOWS
  #if defined(STP2STL_BUILD_DLL)
    #define STP2STL_API __declspec(dllexport)
  #else
    #define STP2STL_API __declspec(dllimport)
  #endif
#elif defined(__GNUC__) || defined(__clang__)
  #define STP2STL_API __attribute__((visibility("default")))
#else
  #define STP2STL_API
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct stp2stl_options {
  double linear_deflection;
  double angular_deflection_deg;
  int relative_deflection;
  int binary;
  double scale;
  int parallel;
} stp2stl_options;

STP2STL_API void (stp2stl_default_options)(stp2stl_options*);

// This library exposes a UTF-8 path interface only: input/output paths must be valid UTF-8 byte sequences.
// Note: platform-specific path handling (e.g. Windows wide paths) should be handled by the caller (e.g. stp2stl_cli).
STP2STL_API int (stp2stl_convert_utf8)(const char*, const char*, const stp2stl_options*);

STP2STL_API const char* (stp2stl_last_error_utf8)(void);
STP2STL_API const char* (stp2stl_version)(void);

#ifdef __cplusplus
}
#endif
