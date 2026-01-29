#include <stp2stl/stp2stl.h>

#include <cstdio>
#include <cmath>
#include <cstring>
#include <string>
#include <string_view>

// OCCT
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <OSD_OpenFile.hxx>
#include <STEPControl_Reader.hxx>
#include <StlAPI_Writer.hxx>
#include <TopoDS_Shape.hxx>
#include <gp_Pnt.hxx>
#include <gp_Trsf.hxx>

#include <Standard_Failure.hxx>

static thread_local std::string g_last_error;

static void set_error(std::string msg)
{
  g_last_error = std::move(msg);
}

static bool is_valid_utf8(std::string_view s)
{
  const auto* p = reinterpret_cast<const unsigned char*>(s.data());
  const size_t n = s.size();
  size_t i = 0;

  while (i < n) {
    const unsigned char c0 = p[i];
    if (c0 <= 0x7F) {
      ++i;
      continue;
    }

    auto is_cont = [&](size_t j) -> bool { return j < n && (p[j] & 0xC0) == 0x80; };

    if (c0 >= 0xC2 && c0 <= 0xDF) { // 2-byte
      if (!is_cont(i + 1)) return false;
      i += 2;
      continue;
    }

    if (c0 == 0xE0) { // 3-byte, no overlong
      if (i + 2 >= n) return false;
      const unsigned char c1 = p[i + 1];
      const unsigned char c2 = p[i + 2];
      if (!(c1 >= 0xA0 && c1 <= 0xBF)) return false;
      if ((c2 & 0xC0) != 0x80) return false;
      i += 3;
      continue;
    }

    if (c0 >= 0xE1 && c0 <= 0xEC) { // 3-byte
      if (!is_cont(i + 1) || !is_cont(i + 2)) return false;
      i += 3;
      continue;
    }

    if (c0 == 0xED) { // 3-byte, no surrogates (U+D800..U+DFFF)
      if (i + 2 >= n) return false;
      const unsigned char c1 = p[i + 1];
      const unsigned char c2 = p[i + 2];
      if (!(c1 >= 0x80 && c1 <= 0x9F)) return false;
      if ((c2 & 0xC0) != 0x80) return false;
      i += 3;
      continue;
    }

    if (c0 >= 0xEE && c0 <= 0xEF) { // 3-byte
      if (!is_cont(i + 1) || !is_cont(i + 2)) return false;
      i += 3;
      continue;
    }

    if (c0 == 0xF0) { // 4-byte, no overlong
      if (i + 3 >= n) return false;
      const unsigned char c1 = p[i + 1];
      if (!(c1 >= 0x90 && c1 <= 0xBF)) return false;
      if (!is_cont(i + 2) || !is_cont(i + 3)) return false;
      i += 4;
      continue;
    }

    if (c0 >= 0xF1 && c0 <= 0xF3) { // 4-byte
      if (!is_cont(i + 1) || !is_cont(i + 2) || !is_cont(i + 3)) return false;
      i += 4;
      continue;
    }

    if (c0 == 0xF4) { // 4-byte, max U+10FFFF
      if (i + 3 >= n) return false;
      const unsigned char c1 = p[i + 1];
      if (!(c1 >= 0x80 && c1 <= 0x8F)) return false;
      if (!is_cont(i + 2) || !is_cont(i + 3)) return false;
      i += 4;
      continue;
    }

    return false;
  }

  return true;
}

void stp2stl_default_options(stp2stl_options* opt)
{
  if (!opt) return;
  opt->linear_deflection = 0.001;
  opt->angular_deflection_deg = 20.0;
  opt->relative_deflection = 1;
  opt->binary = 1;
  opt->scale = 1.0;
  opt->parallel = 0;
}

const char* stp2stl_last_error_utf8(void)
{
  return g_last_error.c_str();
}

const char* stp2stl_version(void)
{
  return "stp2stl/0.1 (OCCT 7.9.3)";
}

static int convert_impl(const std::string& step_path, const std::string& stl_path, const stp2stl_options& opt)
{
  try {
    STEPControl_Reader reader;
    const IFSelect_ReturnStatus stat = reader.ReadFile(step_path.c_str());
    if (stat != IFSelect_RetDone) {
      set_error("STEP read failed: ReadFile returned non-success status");
      return 10;
    }

    if (!reader.TransferRoots()) {
      set_error("STEP read failed: TransferRoots failed");
      return 11;
    }

    TopoDS_Shape shape = reader.OneShape();
    if (shape.IsNull()) {
      set_error("STEP read failed: resulting shape is null");
      return 12;
    }

    if (opt.scale != 1.0) {
      gp_Trsf trsf;
      trsf.SetScale(gp_Pnt(0.0, 0.0, 0.0), opt.scale);
      BRepBuilderAPI_Transform xform(shape, trsf, true);
      shape = xform.Shape();
    }

    const double ang_rad = opt.angular_deflection_deg * (3.14159265358979323846 / 180.0);
    BRepMesh_IncrementalMesh mesher(shape, opt.linear_deflection, opt.relative_deflection != 0, ang_rad, opt.parallel != 0);
    (void)mesher;

    StlAPI_Writer writer;
    writer.ASCIIMode() = (opt.binary == 0);
    writer.Write(shape, stl_path.c_str());

    if (FILE* f = OSD_OpenFile(stl_path.c_str(), "rb")) {
      std::fclose(f);
    } else {
      set_error("STL write failed: output file not created");
      return 20;
    }

    return 0;
  } catch (const Standard_Failure& e) {
    const char* msg = e.GetMessageString();
    set_error(std::string("OCCT exception: ") + (msg ? msg : std::string("Standard_Failure")));
    return 100;
  } catch (...) {
    set_error("Unknown exception");
    return 101;
  }
}

int stp2stl_convert_utf8(const char* step_path_utf8, const char* stl_path_utf8, const stp2stl_options* opt_ptr)
{
  g_last_error.clear();

  if (!step_path_utf8 || !stl_path_utf8) {
    set_error("Null parameter");
    return 1;
  }

  stp2stl_options opt;
  stp2stl_default_options(&opt);
  if (opt_ptr) opt = *opt_ptr;

  const std::string step_u8(step_path_utf8);
  const std::string stl_u8(stl_path_utf8);
  if (step_u8.empty() || stl_u8.empty()) {
    set_error("Empty path");
    return 2;
  }

  if (!is_valid_utf8(step_u8) || !is_valid_utf8(stl_u8)) {
    set_error("Invalid UTF-8 path");
    return 3;
  }

  return convert_impl(step_u8, stl_u8, opt);
}
