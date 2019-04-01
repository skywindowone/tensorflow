/* Copyright 2019 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "tensorflow/lite/delegates/gpu/gl/gpu_info.h"

#include <algorithm>
#include <cctype>
#include <string>

#include "absl/strings/ascii.h"
#include "tensorflow/lite/delegates/gpu/gl/gl_errors.h"
#include "tensorflow/lite/delegates/gpu/gl/portable_gl31.h"

namespace tflite {
namespace gpu {
namespace gl {
namespace {

GpuType GetGpuType(const std::string& renderer) {
  if (renderer.find("mali") != renderer.npos) {
    return GpuType::MALI;
  }
  if (renderer.find("adreno") != renderer.npos) {
    return GpuType::ADRENO;
  }
  if (renderer.find("powervr") != renderer.npos) {
    return GpuType::POWERVR;
  }
  if (renderer.find("intel") != renderer.npos) {
    return GpuType::INTEL;
  }
  if (renderer.find("nvidia") != renderer.npos) {
    return GpuType::NVIDIA;
  }
  return GpuType::UNKNOWN;
}

GpuModel GetGpuModel(const std::string& renderer) {
  auto found_model = [&](std::string model) -> bool {
    return renderer.find(model) != renderer.npos;
  };
  // Adreno 6xx series
  if (found_model("640")) return GpuModel::ADRENO640;
  if (found_model("630")) return GpuModel::ADRENO630;
  if (found_model("616")) return GpuModel::ADRENO616;
  if (found_model("615")) return GpuModel::ADRENO615;
  if (found_model("612")) return GpuModel::ADRENO612;
  if (found_model("605")) return GpuModel::ADRENO605;
  // Adreno 5xx series
  if (found_model("540")) return GpuModel::ADRENO540;
  if (found_model("530")) return GpuModel::ADRENO530;
  if (found_model("512")) return GpuModel::ADRENO512;
  if (found_model("510")) return GpuModel::ADRENO510;
  if (found_model("509")) return GpuModel::ADRENO509;
  if (found_model("508")) return GpuModel::ADRENO508;
  if (found_model("506")) return GpuModel::ADRENO506;
  if (found_model("505")) return GpuModel::ADRENO505;
  if (found_model("504")) return GpuModel::ADRENO504;
  // Adreno 4xx series
  if (found_model("430")) return GpuModel::ADRENO430;
  if (found_model("420")) return GpuModel::ADRENO420;
  if (found_model("418")) return GpuModel::ADRENO418;
  if (found_model("405")) return GpuModel::ADRENO405;
  // Adreno 3xx series
  if (found_model("330")) return GpuModel::ADRENO330;
  if (found_model("320")) return GpuModel::ADRENO320;
  if (found_model("308")) return GpuModel::ADRENO308;
  if (found_model("306")) return GpuModel::ADRENO306;
  if (found_model("305")) return GpuModel::ADRENO305;
  if (found_model("304")) return GpuModel::ADRENO304;
  // Adreno 2xx series
  if (found_model("225")) return GpuModel::ADRENO225;
  if (found_model("220")) return GpuModel::ADRENO220;
  if (found_model("205")) return GpuModel::ADRENO205;
  if (found_model("203")) return GpuModel::ADRENO203;
  if (found_model("200")) return GpuModel::ADRENO200;
  // Adreno 1xx series
  if (found_model("130")) return GpuModel::ADRENO130;
  return GpuModel::UNKNOWN;
}

}  // namespace

void GetGpuModelAndType(const std::string& renderer, GpuModel* gpu_model,
                        GpuType* gpu_type) {
  std::string lowered = renderer;
  absl::AsciiStrToLower(&lowered);
  *gpu_type = GetGpuType(lowered);
  *gpu_model =
      *gpu_type == GpuType::ADRENO ? GetGpuModel(lowered) : GpuModel::UNKNOWN;
}

Status RequestGpuInfo(GpuInfo* gpu_info) {
  GpuInfo info;

  const GLubyte* renderer_name = glGetString(GL_RENDERER);
  if (renderer_name) {
    info.renderer_name = reinterpret_cast<const char*>(renderer_name);
    GetGpuModelAndType(info.renderer_name, &info.gpu_model, &info.type);
  }

  const GLubyte* vendor_name = glGetString(GL_VENDOR);
  if (vendor_name) {
    info.vendor_name = reinterpret_cast<const char*>(vendor_name);
  }

  const GLubyte* version_name = glGetString(GL_VERSION);
  if (version_name) {
    info.version = reinterpret_cast<const char*>(version_name);
  }

  glGetIntegerv(GL_MAJOR_VERSION, &info.major_version);
  glGetIntegerv(GL_MINOR_VERSION, &info.minor_version);

  GLint extensions_count;
  glGetIntegerv(GL_NUM_EXTENSIONS, &extensions_count);
  info.extensions.resize(extensions_count);
  for (int i = 0; i < extensions_count; ++i) {
    info.extensions[i] = std::string(
        reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i)));
  }
  glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, &info.max_ssbo_bindings);
  glGetIntegerv(GL_MAX_COMPUTE_IMAGE_UNIFORMS, &info.max_image_bindings);
  info.max_work_group_size.resize(3);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0,
                  &info.max_work_group_size[0]);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1,
                  &info.max_work_group_size[1]);
  glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2,
                  &info.max_work_group_size[2]);
  glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS,
                &info.max_work_group_invocations);
  glGetIntegerv(GL_MAX_TEXTURE_SIZE, &info.max_texture_size);
  glGetIntegerv(GL_MAX_IMAGE_UNITS, &info.max_image_units);
  glGetIntegerv(GL_MAX_ARRAY_TEXTURE_LAYERS, &info.max_array_texture_layers);
  RETURN_IF_ERROR(GetOpenGlErrors());
  *gpu_info = info;
  return OkStatus();
}

}  // namespace gl
}  // namespace gpu
}  // namespace tflite
