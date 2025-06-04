#include <cstring>
#include <iostream>

#include "navari/navari.h"

int main(int argc, char* argv[]) {
  using namespace navari;

  platform::NavariApp app;
  platform::RenderedBackend rendered_backend;
  platform::LinuxDisplayType linux_display_type;
  material::graph::constants::Constant1Vector cv;

// Auto Determine what best rendered in linux, windows, osx, android & ios and
// the fallback
#if NVR_PLATFORM_LINUX == 1
  // rendered_backend = platform::DetermineRenderedBackend(
  //     platform::RenderedBackend::kVulkan,
  //     platform::RenderedBackend::kOpenGL);

  linux_display_type = platform::DetermineLinuxDisplayType(
      platform::LinuxDisplayType::kWayland, platform::LinuxDisplayType::kX11);

#endif

#if NVR_PLATFORM_WINDOWS == 1
  rendered_backend = platform::DetermineRenderedBackend(
      platform::RenderedBackend::kDirect3D12,
      platform::RenderedBackend::kDirect3D11);

  if (rendered_backend == platform::RenderedBackend::kNone)
    rendered_backend = platform::DetermineRenderedBackend(
        platform::RenderedBackend::kDirect3D11,
        platform::RenderedBackend::kOpenGL);

#endif

#if NVR_PLATFORM_ANDROID == 1
  rendered_backend = platform::DetermineRenderedBackend(
      platform::RenderedBackend::kOpenGLES, platform::RenderedBackend::kOpenGL);
#endif

  auto backends = platform::QuerySupportedBackends();

  for (auto& b : backends) {
    std::cout << platform::ToStringEnumRenderedBackend(b.backend) << ": "
              << b.major_version << "." << b.minor_version << std::endl;
  }

  auto res =
      app.InitApp(platform::RenderedBackend::kOpenGL, "Hello-Stratengine",
                  linux_display_type == platform::LinuxDisplayType::kWayland);

  if (res != platform::NavariAppResult::kOk) {
    std::cerr << "Failed to create Vulkan based engine" << std::endl;
  }

  // Main loop
  bool running = true;
  while (!glfwWindowShouldClose(app.GetWindow())) {
    glfwPollEvents();
    // Rendering code here
    glfwSwapBuffers(app.GetWindow());
  }

  std::cout << "Rendered Backend:   "
            << platform::ToStringEnumRenderedBackend(rendered_backend)
            << std::endl;
  std::cout << "Linux Display:      "
            << platform::ToStringEnumLinuxDisplayType(linux_display_type)
            << std::endl;

  app.ShutdownApp();

  return 0;
}
