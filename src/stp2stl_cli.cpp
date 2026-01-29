#include <stp2stl/stp2stl.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

static void print_usage()
{
  std::fprintf(stderr,
    "Usage:\n"
    "  stp2stl <input.step> <output.stl> [options]\n"
    "\n"
    "Options:\n"
    "  --deflection <v>      Linear deflection (default: 0.001)\n"
    "  --angle <deg>         Angular deflection in degrees (default: 20)\n"
    "  --relative            Use relative deflection (default)\n"
    "  --absolute            Use absolute deflection\n"
    "  --binary              Write binary STL (default)\n"
    "  --ascii               Write ASCII STL\n"
    "  --scale <v>           Scale factor (default: 1.0)\n"
    "  --parallel            Parallel meshing (default: off)\n"
    "  --version             Print version\n"
    "  -h, --help            Help\n"
  );
}

static int parse_double(const char* s, double* out)
{
  if (s == nullptr || *s == '\0') return 0;
  char* end = nullptr;
  const double v = std::strtod(s, &end);
  if (end == s || (end != nullptr && *end != '\0')) return 0;
  *out = v;
  return 1;
}

#if STP2STL_PLATFORM_WINDOWS
#include <cwchar>
#include <string>

#define NOMINMAX
#include <windows.h>

static int wide_to_utf8(const wchar_t* s, std::string* out)
{
  if (out == nullptr) return 0;
  out->clear();
  if (s == nullptr) return 0;
  if (*s == L'\0') return 1;

  const int need = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, s, -1, nullptr, 0, nullptr, nullptr);
  if (need <= 0) return 0;

  std::string tmp(static_cast<size_t>(need), '\0'); // include trailing '\0'
  const int written = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, s, -1, tmp.data(), need, nullptr, nullptr);
  if (written != need) return 0;
  tmp.pop_back(); // drop trailing '\0'

  *out = std::move(tmp);
  return 1;
}

static void print_usage_w()
{
  std::fwprintf(stderr,
    L"Usage:\n"
    L"  stp2stl <input.step> <output.stl> [options]\n"
    L"\n"
    L"Options:\n"
    L"  --deflection <v>      Linear deflection (default: 0.001)\n"
    L"  --angle <deg>         Angular deflection in degrees (default: 20)\n"
    L"  --relative            Use relative deflection (default)\n"
    L"  --absolute            Use absolute deflection\n"
    L"  --binary              Write binary STL (default)\n"
    L"  --ascii               Write ASCII STL\n"
    L"  --scale <v>           Scale factor (default: 1.0)\n"
    L"  --parallel            Parallel meshing (default: off)\n"
    L"  --version             Print version\n"
    L"  -h, --help            Help\n"
  );
}

static int parse_double_w(const wchar_t* s, double* out)
{
  if (s == nullptr || *s == L'\0') return 0;
  wchar_t* end = nullptr;
  const double v = std::wcstod(s, &end);
  if (end == s || (end != nullptr && *end != L'\0')) return 0;
  *out = v;
  return 1;
}

int wmain(int argc, wchar_t** argv)
{
  if (argc < 2) {
    print_usage_w();
    return 2;
  }

  for (int i = 1; i < argc; ++i) {
    if (std::wcscmp(argv[i], L"--help") == 0 || std::wcscmp(argv[i], L"-h") == 0) {
      print_usage_w();
      return 0;
    }
    if (std::wcscmp(argv[i], L"--version") == 0) {
      std::printf("%s\n", stp2stl_version());
      return 0;
    }
  }

  if (argc < 3) {
    print_usage_w();
    return 2;
  }

  const wchar_t* input = argv[1];
  const wchar_t* output = argv[2];

  stp2stl_options opt;
  stp2stl_default_options(&opt);

  for (int i = 3; i < argc; ++i) {
    const wchar_t* a = argv[i];
    if (std::wcscmp(a, L"--deflection") == 0 && i + 1 < argc) {
      double v = 0.0;
      if (!parse_double_w(argv[++i], &v) || v <= 0.0) {
        std::fwprintf(stderr, L"Invalid argument: --deflection expects a positive number\n");
        return 2;
      }
      opt.linear_deflection = v;
      continue;
    }
    if (std::wcscmp(a, L"--angle") == 0 && i + 1 < argc) {
      double v = 0.0;
      if (!parse_double_w(argv[++i], &v) || v <= 0.0) {
        std::fwprintf(stderr, L"Invalid argument: --angle expects a positive number (degrees)\n");
        return 2;
      }
      opt.angular_deflection_deg = v;
      continue;
    }
    if (std::wcscmp(a, L"--relative") == 0) {
      opt.relative_deflection = 1;
      continue;
    }
    if (std::wcscmp(a, L"--absolute") == 0) {
      opt.relative_deflection = 0;
      continue;
    }
    if (std::wcscmp(a, L"--binary") == 0) {
      opt.binary = 1;
      continue;
    }
    if (std::wcscmp(a, L"--ascii") == 0) {
      opt.binary = 0;
      continue;
    }
    if (std::wcscmp(a, L"--scale") == 0 && i + 1 < argc) {
      double v = 0.0;
      if (!parse_double_w(argv[++i], &v) || v <= 0.0) {
        std::fwprintf(stderr, L"Invalid argument: --scale expects a positive number\n");
        return 2;
      }
      opt.scale = v;
      continue;
    }
    if (std::wcscmp(a, L"--parallel") == 0) {
      opt.parallel = 1;
      continue;
    }

    std::fwprintf(stderr, L"Unknown argument: %ls\n", a);
    print_usage_w();
    return 2;
  }

  std::string input_u8;
  std::string output_u8;
  if (!wide_to_utf8(input, &input_u8) || !wide_to_utf8(output, &output_u8)) {
    std::fwprintf(stderr, L"Invalid path encoding (UTF-16 -> UTF-8 conversion failed)\n");
    return 2;
  }

  // stp2stl accepts UTF-8 paths only.
  // On Windows, the CLI gets UTF-16 arguments from wmain, converts them to UTF-8, then calls into the library.
  const int rc = stp2stl_convert_utf8(input_u8.c_str(), output_u8.c_str(), &opt);
  if (rc != 0) {
    std::fprintf(stderr, "Conversion failed (%d): %s\n", rc, stp2stl_last_error_utf8());
    return rc;
  }

  return 0;
}
#else
int main(int argc, char** argv)
{
  if (argc < 2) {
    print_usage();
    return 2;
  }

  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--help") == 0 || std::strcmp(argv[i], "-h") == 0) {
      print_usage();
      return 0;
    }
    if (std::strcmp(argv[i], "--version") == 0) {
      std::printf("%s\n", stp2stl_version());
      return 0;
    }
  }

  if (argc < 3) {
    print_usage();
    return 2;
  }

  const char* input = argv[1];
  const char* output = argv[2];

  stp2stl_options opt;
  stp2stl_default_options(&opt);

  for (int i = 3; i < argc; ++i) {
    const char* a = argv[i];
    if (std::strcmp(a, "--deflection") == 0 && i + 1 < argc) {
      double v = 0.0;
      if (!parse_double(argv[++i], &v) || v <= 0.0) {
        std::fprintf(stderr, "Invalid argument: --deflection expects a positive number\n");
        return 2;
      }
      opt.linear_deflection = v;
      continue;
    }
    if (std::strcmp(a, "--angle") == 0 && i + 1 < argc) {
      double v = 0.0;
      if (!parse_double(argv[++i], &v) || v <= 0.0) {
        std::fprintf(stderr, "Invalid argument: --angle expects a positive number (degrees)\n");
        return 2;
      }
      opt.angular_deflection_deg = v;
      continue;
    }
    if (std::strcmp(a, "--relative") == 0) {
      opt.relative_deflection = 1;
      continue;
    }
    if (std::strcmp(a, "--absolute") == 0) {
      opt.relative_deflection = 0;
      continue;
    }
    if (std::strcmp(a, "--binary") == 0) {
      opt.binary = 1;
      continue;
    }
    if (std::strcmp(a, "--ascii") == 0) {
      opt.binary = 0;
      continue;
    }
    if (std::strcmp(a, "--scale") == 0 && i + 1 < argc) {
      double v = 0.0;
      if (!parse_double(argv[++i], &v) || v <= 0.0) {
        std::fprintf(stderr, "Invalid argument: --scale expects a positive number\n");
        return 2;
      }
      opt.scale = v;
      continue;
    }
    if (std::strcmp(a, "--parallel") == 0) {
      opt.parallel = 1;
      continue;
    }

    std::fprintf(stderr, "Unknown argument: %s\n", a);
    print_usage();
    return 2;
  }

  const int rc = stp2stl_convert_utf8(input, output, &opt);
  if (rc != 0) {
    std::fprintf(stderr, "Conversion failed (%d): %s\n", rc, stp2stl_last_error_utf8());
    return rc;
  }

  return 0;
}
#endif
