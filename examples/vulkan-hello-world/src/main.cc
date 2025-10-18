#include <vulkan/vulkan.h>

#include <cstdio>
#include <vector>



static const char* ApiVersionToStr(uint32_t v) {
  static char buf[64];
  std::snprintf(buf, sizeof(buf), "%u.%u.%u", VK_VERSION_MAJOR(v),
                VK_VERSION_MINOR(v), VK_VERSION_PATCH(v));
  return buf;
}

int main() {
  // Query loader-supported instance version (if 1.1+ loader)
  uint32_t loader_version = VK_API_VERSION_1_0;

  auto enumerate_instance_version_fn =
      reinterpret_cast<PFN_vkEnumerateInstanceVersion>(
          vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"));

  if (enumerate_instance_version_fn) {
    enumerate_instance_version_fn(&loader_version);
  }

  std::printf("[vk] Loader supports: Vulkan %s\n",
              ApiVersionToStr(loader_version));

  // Create a 1.1 instance (baseline target)
  VkApplicationInfo app_info{};
  app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName   = "vk_sanity";
  app_info.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
  app_info.pEngineName        = "quickvulkanv1";
  app_info.engineVersion      = VK_MAKE_VERSION(0, 1, 0);
  app_info.apiVersion         = VK_API_VERSION_1_1;

  VkInstanceCreateInfo instance_ci{};
  instance_ci.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instance_ci.pApplicationInfo = &app_info;

  VkInstance instance = VK_NULL_HANDLE;
  VkResult vk_result  = vkCreateInstance(&instance_ci, nullptr, &instance);
  if (vk_result != VK_SUCCESS) {
    std::fprintf(stderr, "[vk] vkCreateInstance failed: %d\n", vk_result);
    return 1;
  }

  uint32_t physical_device_count = 0;
  vk_result =
      vkEnumeratePhysicalDevices(instance, &physical_device_count, nullptr);
  if (vk_result != VK_SUCCESS || physical_device_count == 0) {
    std::fprintf(stderr, "[vk] No physical devices found (res=%d, count=%u)\n",
                 vk_result, physical_device_count);
    vkDestroyInstance(instance, nullptr);
    return 1;
  }

  std::vector<VkPhysicalDevice> physical_devices(physical_device_count);
  vkEnumeratePhysicalDevices(instance, &physical_device_count,
                             physical_devices.data());

  std::printf("[vk] Found %u physical device(s)\n", physical_device_count);

  for (uint32_t i = 0; i < physical_device_count; ++i) {
    VkPhysicalDeviceProperties device_props{};
    vkGetPhysicalDeviceProperties(physical_devices[i], &device_props);

    std::printf("  - %s | api %s | driver 0x%x | deviceID 0x%04x\n",
                device_props.deviceName,
                ApiVersionToStr(device_props.apiVersion),
                device_props.driverVersion, device_props.deviceID);
  }

  vkDestroyInstance(instance, nullptr);
  std::puts("[vk] Sanity OK.");

  return 0;
}
