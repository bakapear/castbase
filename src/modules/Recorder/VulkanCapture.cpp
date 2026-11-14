#include "VulkanCapture.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_vulkan.h>
#include <libavutil/mem.h>
#include <libavutil/opt.h>
}

static const char* FFMPEG_DLLS[] = {"avutil-61.dll", "avcodec-63.dll", "avformat-63.dll"};

#define REC_LOG(fmt, ...) ConWarning("[Recorder] " fmt "\n", ##__VA_ARGS__)

static constexpr int kCaptureTimeScale = 90000;  // 90 kHz

// // compile: glslc --target-env=vulkan1.0 -O nv12.comp
// layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;
// layout(binding = 0, rgba8) uniform readonly image2D srcRGBA;
// layout(binding = 1, r8) uniform writeonly image2D dstY;
// layout(binding = 2, rg8) uniform writeonly image2D dstUV;
// const vec3 kLuma = vec3(0.299, 0.587, 0.114);
// const vec3 kCb = vec3(-0.168736, -0.331264, 0.5);
// const vec3 kCr = vec3(0.5, -0.418688, -0.081312);
// void main() {
//   ivec2 size = imageSize(srcRGBA);
//   ivec2 p = ivec2(gl_GlobalInvocationID.xy);
//   if (p.x >= size.x || p.y >= size.y) return;
//   vec3 c00 = imageLoad(srcRGBA, p).rgb;
//   float y = 16.0 / 255.0 + (219.0 / 255.0) * dot(c00, kLuma);
//   imageStore(dstY, p, vec4(y, 0.0, 0.0, 0.0));
//   if (((p.x | p.y) & 1) == 0) {
//     ivec2 px = ivec2(min(p.x + 1, size.x - 1), p.y);
//     ivec2 py = ivec2(p.x, min(p.y + 1, size.y - 1));
//     ivec2 pxy = ivec2(min(p.x + 1, size.x - 1), min(p.y + 1, size.y - 1));
//     vec3 avg = (c00 + imageLoad(srcRGBA, px).rgb + imageLoad(srcRGBA, py).rgb + imageLoad(srcRGBA, pxy).rgb) * 0.25;
//     float cb = 128.0 / 255.0 + (224.0 / 255.0) * dot(avg, kCb);
//     float cr = 128.0 / 255.0 + (224.0 / 255.0) * dot(avg, kCr);
//     imageStore(dstUV, ivec2(p.x >> 1, p.y >> 1), vec4(cb, cr, 0.0, 0.0));
//   }
// }
static const uint32_t kNV12ComputeSpv[] = {0x07230203, 0x00010000, 0x000d000b, 0x000000b3, 0x00000000, 0x00020011, 0x00000001, 0x00020011, 0x00000031, 0x00020011, 0x00000032, 0x0006000b, 0x00000001, 0x4c534c47, 0x6474732e, 0x3035342e, 0x00000000, 0x0003000e, 0x00000000, 0x00000001, 0x0006000f, 0x00000005, 0x00000004, 0x6e69616d, 0x00000000, 0x00000014, 0x00060010, 0x00000004, 0x00000011, 0x00000008, 0x00000008, 0x00000001, 0x00030047, 0x0000000d, 0x00000018, 0x00040047, 0x0000000d, 0x00000021, 0x00000000, 0x00040047, 0x0000000d, 0x00000022, 0x00000000, 0x00040047, 0x00000014, 0x0000000b, 0x0000001c, 0x00030047, 0x00000044, 0x00000019, 0x00040047, 0x00000044, 0x00000021, 0x00000001, 0x00040047, 0x00000044, 0x00000022, 0x00000000, 0x00030047, 0x000000a3, 0x00000019, 0x00040047, 0x000000a3, 0x00000021, 0x00000002, 0x00040047, 0x000000a3, 0x00000022, 0x00000000, 0x00040047, 0x000000b0, 0x0000000b, 0x00000019, 0x00020013, 0x00000002, 0x00030021, 0x00000003, 0x00000002, 0x00040015, 0x00000006, 0x00000020, 0x00000001, 0x00040017, 0x00000007, 0x00000006, 0x00000002, 0x00030016, 0x0000000a, 0x00000020, 0x00090019, 0x0000000b, 0x0000000a, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000002, 0x00000004, 0x00040020, 0x0000000c, 0x00000000, 0x0000000b, 0x0004003b, 0x0000000c, 0x0000000d, 0x00000000, 0x00040015, 0x00000011, 0x00000020, 0x00000000, 0x00040017, 0x00000012, 0x00000011, 0x00000003, 0x00040020, 0x00000013, 0x00000001, 0x00000012, 0x0004003b, 0x00000013, 0x00000014, 0x00000001, 0x00040017, 0x00000015, 0x00000011, 0x00000002, 0x00020014, 0x00000019, 0x0004002b, 0x00000011, 0x0000001a, 0x00000000, 0x0004002b, 0x00000011, 0x00000024, 0x00000001, 0x00040017, 0x0000002e, 0x0000000a, 0x00000003, 0x00040017, 0x00000033, 0x0000000a, 0x00000004, 0x0004002b, 0x0000000a, 0x00000038, 0x3d808081, 0x0004002b, 0x0000000a, 0x00000039, 0x3f5bdbdc, 0x0004002b, 0x0000000a, 0x0000003b, 0x3e991687, 0x0004002b, 0x0000000a, 0x0000003c, 0x3f1645a2, 0x0004002b, 0x0000000a, 0x0000003d, 0x3de978d5, 0x0006002c, 0x0000002e, 0x0000003e, 0x0000003b, 0x0000003c, 0x0000003d, 0x00090019, 0x00000042, 0x0000000a, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000002, 0x0000000f, 0x00040020, 0x00000043, 0x00000000, 0x00000042, 0x0004003b, 0x00000043, 0x00000044, 0x00000000, 0x0004002b, 0x0000000a, 0x00000048, 0x00000000, 0x0004002b, 0x00000006, 0x0000004f, 0x00000001, 0x0004002b, 0x00000006, 0x00000051, 0x00000000, 0x0004002b, 0x0000000a, 0x0000008c, 0x3e800000, 0x0004002b, 0x0000000a, 0x0000008f, 0x3f008081, 0x0004002b, 0x0000000a, 0x00000090, 0x3f60e0e1, 0x0004002b, 0x0000000a, 0x00000092, 0xbe2cc921, 0x0004002b, 0x0000000a, 0x00000093, 0xbea99b6f, 0x0004002b, 0x0000000a, 0x00000094, 0x3f000000, 0x0006002c, 0x0000002e, 0x00000095, 0x00000092, 0x00000093, 0x00000094, 0x0004002b, 0x0000000a, 0x0000009b, 0xbed65e46, 0x0004002b, 0x0000000a, 0x0000009c, 0xbda686e8, 0x0006002c, 0x0000002e, 0x0000009d, 0x00000094, 0x0000009b, 0x0000009c, 0x00090019, 0x000000a1, 0x0000000a, 0x00000001, 0x00000000, 0x00000000, 0x00000000, 0x00000002, 0x0000000d, 0x00040020, 0x000000a2, 0x00000000, 0x000000a1, 0x0004003b, 0x000000a2, 0x000000a3, 0x00000000, 0x0004002b, 0x00000011, 0x000000af, 0x00000008, 0x0006002c, 0x00000012, 0x000000b0, 0x000000af, 0x000000af, 0x00000024, 0x00050036, 0x00000002, 0x00000004, 0x00000000, 0x00000003, 0x000200f8, 0x00000005, 0x000300f7, 0x000000b1, 0x00000000, 0x000300fb, 0x0000001a, 0x000000b2, 0x000200f8, 0x000000b2, 0x0004003d, 0x0000000b, 0x0000000e, 0x0000000d, 0x00040068, 0x00000007, 0x0000000f, 0x0000000e, 0x0004003d, 0x00000012, 0x00000016, 0x00000014, 0x0007004f, 0x00000015, 0x00000017, 0x00000016, 0x00000016, 0x00000000, 0x00000001, 0x0004007c, 0x00000007, 0x00000018, 0x00000017, 0x00050051, 0x00000006, 0x0000001d, 0x00000018, 0x00000000, 0x00050051, 0x00000006, 0x0000001f, 0x0000000f, 0x00000000, 0x000500af, 0x00000019, 0x00000020, 0x0000001d, 0x0000001f, 0x000400a8, 0x00000019, 0x00000021, 0x00000020, 0x000300f7, 0x00000023, 0x00000000, 0x000400fa, 0x00000021, 0x00000022, 0x00000023, 0x000200f8, 0x00000022, 0x00050051, 0x00000006, 0x00000026, 0x00000018, 0x00000001, 0x00050051, 0x00000006, 0x00000028, 0x0000000f, 0x00000001, 0x000500af, 0x00000019, 0x00000029, 0x00000026, 0x00000028, 0x000200f9, 0x00000023, 0x000200f8, 0x00000023, 0x000700f5, 0x00000019, 0x0000002a, 0x00000020, 0x000000b2, 0x00000029, 0x00000022, 0x000300f7, 0x0000002c, 0x00000000, 0x000400fa, 0x0000002a, 0x0000002b, 0x0000002c, 0x000200f8, 0x0000002b, 0x000200f9, 0x000000b1, 0x000200f8, 0x0000002c, 0x0004003d, 0x0000000b, 0x00000031, 0x0000000d, 0x00050062, 0x00000033, 0x00000034, 0x00000031, 0x00000018, 0x0008004f, 0x0000002e, 0x00000035, 0x00000034, 0x00000034, 0x00000000, 0x00000001, 0x00000002, 0x00050094, 0x0000000a, 0x0000003f, 0x00000035, 0x0000003e, 0x00050085, 0x0000000a, 0x00000040, 0x00000039, 0x0000003f, 0x00050081, 0x0000000a, 0x00000041, 0x00000038, 0x00000040, 0x0004003d, 0x00000042, 0x00000045, 0x00000044, 0x00070050, 0x00000033, 0x00000049, 0x00000041, 0x00000048, 0x00000048, 0x00000048, 0x00040063, 0x00000045, 0x00000018, 0x00000049, 0x00050051, 0x00000006, 0x0000004d, 0x00000018, 0x00000001, 0x000500c5, 0x00000006, 0x0000004e, 0x0000001d, 0x0000004d, 0x000500c7, 0x00000006, 0x00000050, 0x0000004e, 0x0000004f, 0x000500aa, 0x00000019, 0x00000052, 0x00000050, 0x00000051, 0x000300f7, 0x00000054, 0x00000000, 0x000400fa, 0x00000052, 0x00000053, 0x00000054, 0x000200f8, 0x00000053, 0x00050080, 0x00000006, 0x00000058, 0x0000001d, 0x0000004f, 0x00050082, 0x00000006, 0x0000005b, 0x0000001f, 0x0000004f, 0x0007000c, 0x00000006, 0x0000005c, 0x00000001, 0x00000027, 0x00000058, 0x0000005b, 0x00050050, 0x00000007, 0x0000005f, 0x0000005c, 0x0000004d, 0x00050080, 0x00000006, 0x00000065, 0x0000004d, 0x0000004f, 0x00050051, 0x00000006, 0x00000067, 0x0000000f, 0x00000001, 0x00050082, 0x00000006, 0x00000068, 0x00000067, 0x0000004f, 0x0007000c, 0x00000006, 0x00000069, 0x00000001, 0x00000027, 0x00000065, 0x00000068, 0x00050050, 0x00000007, 0x0000006a, 0x0000001d, 0x00000069, 0x00050050, 0x00000007, 0x0000007a, 0x0000005c, 0x00000069, 0x0004003d, 0x0000000b, 0x0000007d, 0x0000000d, 0x00050062, 0x00000033, 0x0000007f, 0x0000007d, 0x0000005f, 0x0008004f, 0x0000002e, 0x00000080, 0x0000007f, 0x0000007f, 0x00000000, 0x00000001, 0x00000002, 0x00050081, 0x0000002e, 0x00000081, 0x00000035, 0x00000080, 0x0004003d, 0x0000000b, 0x00000082, 0x0000000d, 0x00050062, 0x00000033, 0x00000084, 0x00000082, 0x0000006a, 0x0008004f, 0x0000002e, 0x00000085, 0x00000084, 0x00000084, 0x00000000, 0x00000001, 0x00000002, 0x00050081, 0x0000002e, 0x00000086, 0x00000081, 0x00000085, 0x0004003d, 0x0000000b, 0x00000087, 0x0000000d, 0x00050062, 0x00000033, 0x00000089, 0x00000087, 0x0000007a, 0x0008004f, 0x0000002e, 0x0000008a, 0x00000089, 0x00000089, 0x00000000, 0x00000001, 0x00000002, 0x00050081, 0x0000002e, 0x0000008b, 0x00000086, 0x0000008a, 0x0005008e, 0x0000002e, 0x0000008d, 0x0000008b, 0x0000008c, 0x00050094, 0x0000000a, 0x00000096, 0x0000008d, 0x00000095, 0x00050085, 0x0000000a, 0x00000097, 0x00000090, 0x00000096, 0x00050081, 0x0000000a, 0x00000098, 0x0000008f, 0x00000097, 0x00050094, 0x0000000a, 0x0000009e, 0x0000008d, 0x0000009d, 0x00050085, 0x0000000a, 0x0000009f, 0x00000090, 0x0000009e, 0x00050081, 0x0000000a, 0x000000a0, 0x0000008f, 0x0000009f, 0x0004003d, 0x000000a1, 0x000000a4, 0x000000a3, 0x000500c3, 0x00000006, 0x000000a7, 0x0000001d, 0x0000004f, 0x000500c3, 0x00000006, 0x000000aa, 0x0000004d, 0x0000004f, 0x00050050, 0x00000007, 0x000000ab, 0x000000a7, 0x000000aa, 0x00070050, 0x00000033, 0x000000ae, 0x00000098, 0x000000a0, 0x00000048, 0x00000048, 0x00040063, 0x000000a4, 0x000000ab, 0x000000ae, 0x000200f9, 0x00000054, 0x000200f8, 0x00000054, 0x000200f9, 0x000000b1, 0x000200f8, 0x000000b1, 0x000100fd, 0x00010038};

static bool LoadFFMpegDlls() {
  HMODULE self = nullptr;
  if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCSTR>(&LoadFFMpegDlls), &self)) {
    REC_LOG("ERROR: Could not locate the plugin dll");
    return false;
  }

  char path[MAX_PATH];
  const DWORD len = GetModuleFileNameA(self, path, MAX_PATH);
  if (len == 0 || len >= MAX_PATH) return false;

  std::string dir(path, len);
  dir.resize(dir.find_last_of("\\/") + 1);
  dir += "ffmpeg\\";

  if (GetFileAttributesA(dir.c_str()) == INVALID_FILE_ATTRIBUTES) {
    REC_LOG("ERROR: ffmpeg folder not found: %s", dir.c_str());
    return false;
  }

  for (const char* name : FFMPEG_DLLS) {
    const std::string full = dir + name;
    if (!LoadLibraryExA(full.c_str(), nullptr, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS)) {
      REC_LOG("ERROR: Failed to load %s (error %lu)", full.c_str(), GetLastError());  // 126 = a dependency is missing, 193 = wrong bitness
      return false;
    }
  }
  return true;
}

static uint32_t FindMemoryType(const VkPhysicalDeviceMemoryProperties& memProps, uint32_t typeBits, VkMemoryPropertyFlags required) {
  for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
    if ((typeBits & (1u << i)) && (memProps.memoryTypes[i].propertyFlags & required) == required) return i;
  }
  for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
    if (typeBits & (1u << i)) return i;
  }
  return 0;
}

VkResult VKAPI_CALL VulkanCapture::QueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo) {
  auto fn = hookQueuePresentKHR.GetTrampoline(&VulkanCapture::QueuePresentKHR);
  if (!recording.load()) return fn(queue, pPresentInfo);

  VkSemaphore captureDone = VK_NULL_HANDLE;
  {
    std::lock_guard<std::mutex> guard(mutex);
    if (recording.load()) {
      for (uint32_t i = 0; i < pPresentInfo->swapchainCount; ++i) {
        if (pPresentInfo->pSwapchains[i] != swapchain) continue;
        CaptureFrameLocked(queue, pPresentInfo->pImageIndices[i], pPresentInfo, &captureDone);
        break;
      }
    }
  }

  if (captureDone == VK_NULL_HANDLE) return fn(queue, pPresentInfo);

  VkPresentInfoKHR modified = *pPresentInfo;
  modified.waitSemaphoreCount = 1;
  modified.pWaitSemaphores = &captureDone;
  return fn(queue, &modified);
}

VkResult VKAPI_CALL VulkanCapture::CreateSwapchainKHR(VkDevice vkDevice, const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain) {
  auto fn = hookCreateSwapchainKHR.GetTrampoline(&VulkanCapture::CreateSwapchainKHR);

  VkSwapchainCreateInfoKHR modified = *pCreateInfo;
  modified.imageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  // modified.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR; // no vsync

  {
    std::lock_guard<std::mutex> guard(mutex);
    device = vkDevice;
    FlushPendingFrame();
  }

  VkResult result = fn(vkDevice, &modified, pAllocator, pSwapchain);
  if (result != VK_SUCCESS || !pSwapchain || !*pSwapchain) return result;

  std::lock_guard<std::mutex> guard(mutex);

  swapchain = *pSwapchain;
  swapchainFormat = pCreateInfo->imageFormat;
  width = pCreateInfo->imageExtent.width;
  height = pCreateInfo->imageExtent.height;

  uint32_t imageCount = 0;
  vkGetSwapchainImagesKHR(vkDevice, swapchain, &imageCount, nullptr);
  swapchainImages.resize(imageCount);
  if (imageCount > 0) vkGetSwapchainImagesKHR(vkDevice, swapchain, &imageCount, swapchainImages.data());

  retiredSemaphores.insert(retiredSemaphores.end(), presentSemaphores.begin(), presentSemaphores.end());
  presentSemaphores.clear();
  if (recording.load()) EnsurePresentSemaphoresLocked();

  if (!initialized) {
    initialized = true;
    if (onInitialized) onInitialized();
  }

  return result;
}

// https://github.com/bruhmoment21/UniversalHookX/blob/1e5496720c0493a9b79360fb82f8cca512eff956/UniversalHookX/src/hooks/backend/vulkan/hook_vulkan.cpp#L47
bool VulkanCapture::CreateFakeDevice() {
  // create vulkan instance
  {
    VkInstanceCreateInfo create_info = {};
    constexpr const char* instance_extension = "VK_KHR_surface";

    create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.enabledExtensionCount = 1;
    create_info.ppEnabledExtensionNames = &instance_extension;

    if (vkCreateInstance(&create_info, allocator, &instance) != VK_SUCCESS) {
      REC_LOG("ERROR: vkCreateInstance failed");
      instance = VK_NULL_HANDLE;
      return false;
    }
  }

  // select GPU (discrete if there is one)
  {
    uint32_t gpu_count = 0;
    vkEnumeratePhysicalDevices(instance, &gpu_count, NULL);
    if (gpu_count == 0) {
      REC_LOG("ERROR: no Vulkan physical devices");
      return false;
    }

    std::vector<VkPhysicalDevice> gpus(gpu_count);
    vkEnumeratePhysicalDevices(instance, &gpu_count, gpus.data());

    int use_gpu = 0;
    for (int i = 0; i < (int)gpu_count; ++i) {
      VkPhysicalDeviceProperties properties;
      vkGetPhysicalDeviceProperties(gpus[i], &properties);
      if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        use_gpu = i;
        break;
      }
    }
    physicalDevice = gpus[use_gpu];
  }

  // check for storage support
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
  {
    VkFormatProperties props{};
    vkGetPhysicalDeviceFormatProperties(physicalDevice, VK_FORMAT_R8G8B8A8_UNORM, &props);
    rgba8StorageSupported = (props.optimalTilingFeatures & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) != 0;
  }

  // select graphics queue family
  {
    uint32_t count;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, NULL);
    queueFamilies.resize(count);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &count, queueFamilies.data());
    for (uint32_t i = 0; i < count; ++i) {
      if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        queueFamily = i;
        break;
      }
    }
    if (queueFamily == (uint32_t)-1) {
      REC_LOG("ERROR: no graphics queue family");
      return false;
    }
  }

  // create logical device
  {
    constexpr const char* device_extension = "VK_KHR_swapchain";
    constexpr const float queue_priority = 1.0f;

    VkDeviceQueueCreateInfo queue_info = {};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = queueFamily;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = &queue_priority;

    VkDeviceCreateInfo create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.queueCreateInfoCount = 1;
    create_info.pQueueCreateInfos = &queue_info;
    create_info.enabledExtensionCount = 1;
    create_info.ppEnabledExtensionNames = &device_extension;

    if (vkCreateDevice(physicalDevice, &create_info, allocator, &fakeDevice) != VK_SUCCESS) {
      REC_LOG("ERROR: vkCreateDevice (fake) failed");
      fakeDevice = VK_NULL_HANDLE;
      return false;
    }
  }

  return true;
}

void VulkanCapture::DestroyFakeDevice() {
  if (fakeDevice) {
    vkDestroyDevice(fakeDevice, allocator);
    fakeDevice = VK_NULL_HANDLE;
  }

  if (instance) {
    vkDestroyInstance(instance, allocator);
    instance = VK_NULL_HANDLE;
  }

  physicalDevice = VK_NULL_HANDLE;
}

void VulkanCapture::Load() {
  ffmpegAvailable = LoadFFMpegDlls();
  if (!ffmpegAvailable) {
    REC_LOG("FFmpeg not available, recorder disabled");
    return;  // no hooks installed, the game is untouched
  }

  CreateFakeDevice();

  void* fnQueuePresentKHR = reinterpret_cast<void*>(vkGetDeviceProcAddr(fakeDevice, "vkQueuePresentKHR"));
  hookQueuePresentKHR.Install(fnQueuePresentKHR, &VulkanCapture::QueuePresentKHR, this);

  void* fnCreateSwapchainKHR = reinterpret_cast<void*>(vkGetDeviceProcAddr(fakeDevice, "vkCreateSwapchainKHR"));
  hookCreateSwapchainKHR.Install(fnCreateSwapchainKHR, &VulkanCapture::CreateSwapchainKHR, this);

  DestroyFakeDevice();
}

void VulkanCapture::Unload() {
  Stop();

  hookQueuePresentKHR.Remove();
  hookCreateSwapchainKHR.Remove();

  std::lock_guard<std::mutex> guard(mutex);
  if (device) {
    vkDeviceWaitIdle(device);
    for (VkSemaphore s : presentSemaphores) vkDestroySemaphore(device, s, nullptr);
    for (VkSemaphore s : retiredSemaphores) vkDestroySemaphore(device, s, nullptr);
  }
  presentSemaphores.clear();
  retiredSemaphores.clear();
  swapchainImages.clear();
  swapchain = VK_NULL_HANDLE;
  device = VK_NULL_HANDLE;

  if (hwDeviceCtx) av_buffer_unref(&hwDeviceCtx);
}

bool VulkanCapture::Start(const char* filename, int movieFps, const char* movieEncoder, int movieQP) {
  std::lock_guard<std::mutex> guard(mutex);
  return StartLocked(filename, movieFps, movieEncoder, movieQP);
}

void VulkanCapture::Stop() {
  std::lock_guard<std::mutex> guard(mutex);
  StopLocked();
}

bool VulkanCapture::StartLocked(const char* filename, int movieFps, const char* movieEncoder, int movieQP) {
  if (recording.load()) return false;
  if (!filename || !*filename) return false;
  if (swapchain == VK_NULL_HANDLE || !device || swapchainImages.empty()) return false;

  basePath = filename;
  if (basePath.size() < 4 || _stricmp(basePath.c_str() + basePath.size() - 4, ".mp4") != 0) basePath += ".mp4";
  fps = movieFps > 1000 ? 1000 : (movieFps > 0 ? movieFps : 0);
  encoder = movieEncoder;
  qp = movieQP;

  if (!EnsurePresentSemaphoresLocked()) return false;

  if (!EnsureEncoder(width, height)) {
    CloseEncoderLocked();
    return false;
  }

  recording = true;
  return true;
}

void VulkanCapture::StopLocked() {
  if (!recording.load() && !encoderCtx && !formatCtx && !conv.cmdPool) return;
  recording = false;

  CloseEncoderLocked();
  DestroyConverter();
}

bool VulkanCapture::EnsurePresentSemaphoresLocked() {
  if (presentSemaphores.size() == swapchainImages.size()) return true;

  VkSemaphoreCreateInfo info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  while (presentSemaphores.size() < swapchainImages.size()) {
    VkSemaphore s = VK_NULL_HANDLE;
    if (vkCreateSemaphore(device, &info, nullptr, &s) != VK_SUCCESS) {
      REC_LOG("ERROR: vkCreateSemaphore failed");
      return false;
    }
    presentSemaphores.push_back(s);
  }
  return true;
}

// create mp4 container and write header
bool VulkanCapture::OpenMuxer() {
  const std::string path = basePath;

  int ret = avformat_alloc_output_context2(&formatCtx, nullptr, nullptr, path.c_str());
  if (ret < 0 || !formatCtx) {
    REC_LOG("[MP4] ERROR: Could not create output context for %s", path.c_str());
    return false;
  }

  videoStream = avformat_new_stream(formatCtx, nullptr);
  if (!videoStream) {
    REC_LOG("[MP4] ERROR: avformat_new_stream failed");
    avformat_free_context(formatCtx);
    formatCtx = nullptr;
    return false;
  }

  if (avcodec_parameters_from_context(videoStream->codecpar, encoderCtx) < 0) {
    REC_LOG("[MP4] ERROR: avcodec_parameters_from_context failed");
    avformat_free_context(formatCtx);
    formatCtx = nullptr;
    videoStream = nullptr;
    return false;
  }

  videoStream->time_base = encoderCtx->time_base;
  videoStream->avg_frame_rate = encoderCtx->framerate;

  if (!(formatCtx->oformat->flags & AVFMT_NOFILE)) {
    if (avio_open(&formatCtx->pb, path.c_str(), AVIO_FLAG_WRITE) < 0) {
      REC_LOG("[MP4] ERROR: Could not open %s", path.c_str());
      avformat_free_context(formatCtx);
      formatCtx = nullptr;
      videoStream = nullptr;
      return false;
    }
  }

  AVDictionary* muxOpts = nullptr;
  av_dict_set(&muxOpts, "movflags", "frag_keyframe+empty_moov", 0);

  if (avformat_write_header(formatCtx, &muxOpts) < 0) {
    REC_LOG("[MP4] ERROR: avformat_write_header failed for %s", path.c_str());
    av_dict_free(&muxOpts);
    if (formatCtx->pb) avio_closep(&formatCtx->pb);
    avformat_free_context(formatCtx);
    formatCtx = nullptr;
    videoStream = nullptr;
    return false;
  }
  av_dict_free(&muxOpts);

  return true;
}

void VulkanCapture::CloseMuxer() {
  if (!formatCtx) return;

  int ret = av_write_trailer(formatCtx);
  if (ret < 0) REC_LOG("[MP4] WARNING: av_write_trailer failed: %d", ret);
  if (formatCtx->pb) avio_closep(&formatCtx->pb);
  avformat_free_context(formatCtx);

  formatCtx = nullptr;
  videoStream = nullptr;
}

int VulkanCapture::WritePackets() {
  if (!packet && !(packet = av_packet_alloc())) return 0;
  if (!encoderCtx) return 0;

  int written = 0;
  while (avcodec_receive_packet(encoderCtx, packet) >= 0) {
    if (formatCtx && videoStream) {
      av_packet_rescale_ts(packet, encoderCtx->time_base, videoStream->time_base);
      packet->stream_index = videoStream->index;

      if (packet->pts == AV_NOPTS_VALUE) packet->pts = 0;
      if (packet->dts == AV_NOPTS_VALUE) packet->dts = packet->pts;
      if (packet->duration <= 0) packet->duration = av_rescale_q(1, AVRational{1, fps > 0 ? fps : 60}, videoStream->time_base);

      int ret = av_interleaved_write_frame(formatCtx, packet);
      if (ret < 0) {
        static std::atomic<bool> logged(false);
        if (!logged.exchange(true)) REC_LOG("[MP4] ERROR: av_interleaved_write_frame failed: %d", ret);
      } else {
        ++written;
      }
    }
    av_packet_unref(packet);
  }
  return written;
}

void VulkanCapture::FlushPendingFrame() {
  if (!conv.pendingFrame) return;

  if (conv.captureFence && device) vkWaitForFences(device, 1, &conv.captureFence, VK_TRUE, UINT64_MAX);

  if (!encoderCtx) {
    av_frame_free(&conv.pendingFrame);
    return;
  }

  conv.pendingFrame->pts = conv.pendingPts;
  conv.pendingFrame->duration = conv.pendingDuration;

  int sendRet;
  while ((sendRet = avcodec_send_frame(encoderCtx, conv.pendingFrame)) == AVERROR(EAGAIN)) {
    if (WritePackets() == 0) std::this_thread::yield();
  }
  if (sendRet < 0) {
    static std::atomic<int> sendErrCount(0);
    if (sendErrCount.fetch_add(1) == 0) REC_LOG("[FFmpeg] ERROR: avcodec_send_frame failed: %d", sendRet);
  }
  WritePackets();

  av_frame_free(&conv.pendingFrame);
}

void VulkanCapture::CloseEncoderLocked() {
  FlushPendingFrame();
  DestroyDstPlaneViews();

  if (encoderCtx) {
    while (avcodec_send_frame(encoderCtx, nullptr) == AVERROR(EAGAIN)) {
      if (WritePackets() == 0) break;
    }
    WritePackets();
    avcodec_free_context(&encoderCtx);
  }

  av_packet_free(&packet);
  CloseMuxer();

  encoderReady = false;
  encoderWidth = 0;
  encoderHeight = 0;
  captureStartValid = false;
  lastPts = -1;
  frameIndex = 0;
}

bool VulkanCapture::EnsureEncoder(uint32_t w, uint32_t h) {
  if (encoderReady && encoderWidth == w && encoderHeight == h) return true;
  if (w == 0 || h == 0) return false;

  if (!hwDeviceCtx) {
    if (av_hwdevice_ctx_create(&hwDeviceCtx, AV_HWDEVICE_TYPE_VULKAN, nullptr, nullptr, 0) < 0) {
      REC_LOG("[FFmpeg] ERROR: av_hwdevice_ctx_create failed");
      return false;
    }
  }

  const AVCodec* codec = avcodec_find_encoder_by_name(encoder);
  if (!codec) {
    REC_LOG("[FFmpeg] ERROR: codec '%s' not found", encoder);
    return false;
  }

  encoderCtx = avcodec_alloc_context3(codec);
  if (!encoderCtx) {
    REC_LOG("[FFmpeg] ERROR: Failed to allocate encoder context");
    return false;
  }

  av_opt_set(encoderCtx->priv_data, "tune", "hq", 0);
  av_opt_set(encoderCtx->priv_data, "usage", "record", 0);
  av_opt_set(encoderCtx->priv_data, "content", "rendered", 0);

  const char* depthEnv = getenv("CAPTURE_ASYNC_DEPTH");
  int asyncDepth = depthEnv ? atoi(depthEnv) : 6;
  if (asyncDepth < 1) asyncDepth = 1;
  if (asyncDepth > 32) asyncDepth = 32;
  {
    char depthStr[16];
    _snprintf_s(depthStr, sizeof(depthStr), _TRUNCATE, "%d", asyncDepth);
    if (av_opt_set(encoderCtx->priv_data, "async_depth", depthStr, 0) < 0) {
      REC_LOG("[FFmpeg] async_depth not supported by %s", encoder);
    }
  }

  encoderCtx->width = w;
  encoderCtx->height = h;

  encoderCtx->time_base = {1, kCaptureTimeScale};
  encoderCtx->framerate = fps > 0 ? AVRational{fps, 1} : AVRational{60, 1};
  encoderCtx->pix_fmt = AV_PIX_FMT_VULKAN;
  encoderCtx->sw_pix_fmt = AV_PIX_FMT_NV12;
  encoderCtx->hw_device_ctx = av_buffer_ref(hwDeviceCtx);
  encoderCtx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  encoderCtx->global_quality = qp;
  encoderCtx->bit_rate = 50000000;  // fallback
  encoderCtx->gop_size = fps > 0 ? fps : 60;

  encoderCtx->color_range = AVCOL_RANGE_MPEG;
  encoderCtx->colorspace = AVCOL_SPC_SMPTE170M;
  encoderCtx->color_primaries = AVCOL_PRI_SMPTE170M;
  encoderCtx->color_trc = AVCOL_TRC_SMPTE170M;
  encoderCtx->chroma_sample_location = AVCHROMA_LOC_CENTER;

  AVBufferRef* hw_frames_ref = av_hwframe_ctx_alloc(hwDeviceCtx);
  if (!hw_frames_ref) {
    REC_LOG("[FFmpeg] ERROR: Failed to allocate AVHWFramesContext ref");
    avcodec_free_context(&encoderCtx);
    return false;
  }

  AVHWFramesContext* frames_ctx = (AVHWFramesContext*)(hw_frames_ref->data);
  frames_ctx->format = AV_PIX_FMT_VULKAN;
  frames_ctx->sw_format = AV_PIX_FMT_NV12;
  frames_ctx->width = w;
  frames_ctx->height = h;
  frames_ctx->initial_pool_size = asyncDepth + 6;

  if (av_hwframe_ctx_init(hw_frames_ref) < 0) {
    REC_LOG("[FFmpeg] ERROR: Failed to initialize AVHWFramesContext for %ux%u", w, h);
    av_buffer_unref(&hw_frames_ref);
    avcodec_free_context(&encoderCtx);
    return false;
  }

  encoderCtx->hw_frames_ctx = av_buffer_ref(hw_frames_ref);
  av_buffer_unref(&hw_frames_ref);

  if (avcodec_open2(encoderCtx, codec, nullptr) < 0) {
    REC_LOG("[FFmpeg] ERROR: avcodec_open2 failed for %ux%u", w, h);
    avcodec_free_context(&encoderCtx);
    return false;
  }

  if (!OpenMuxer()) {
    avcodec_free_context(&encoderCtx);
    return false;
  }

  encoderWidth = w;
  encoderHeight = h;
  encoderReady = true;
  return true;
}

bool VulkanCapture::InitConverter() {
  if (conv.initialized) return true;
  if (conv.disabled) return false;

  if (!rgba8StorageSupported) {
    REC_LOG("[NV12] ERROR: R8G8B8A8_UNORM storage images unsupported");
    conv.disabled = true;
    return false;
  }

  VkCommandPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
  poolInfo.queueFamilyIndex = queueFamily;
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  if (vkCreateCommandPool(device, &poolInfo, nullptr, &conv.cmdPool) != VK_SUCCESS) {
    REC_LOG("[NV12] ERROR: vkCreateCommandPool failed");
    conv.disabled = true;
    return false;
  }

  VkCommandBufferAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
  allocInfo.commandPool = conv.cmdPool;
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = 1;
  if (vkAllocateCommandBuffers(device, &allocInfo, &conv.cmdBuf) != VK_SUCCESS) {
    REC_LOG("[NV12] ERROR: vkAllocateCommandBuffers failed");
    conv.disabled = true;
    return false;
  }

  VkFenceCreateInfo fenceInfo = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO};
  if (vkCreateFence(device, &fenceInfo, nullptr, &conv.captureFence) != VK_SUCCESS) {
    REC_LOG("[NV12] ERROR: vkCreateFence failed");
    conv.disabled = true;
    return false;
  }

  VkShaderModuleCreateInfo moduleInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
  moduleInfo.codeSize = sizeof(kNV12ComputeSpv);
  moduleInfo.pCode = kNV12ComputeSpv;

  VkShaderModule shaderModule = VK_NULL_HANDLE;
  if (vkCreateShaderModule(device, &moduleInfo, nullptr, &shaderModule) != VK_SUCCESS) {
    REC_LOG("[NV12] ERROR: vkCreateShaderModule failed");
    conv.disabled = true;
    return false;
  }

  VkDescriptorSetLayoutBinding bindings[3] = {};
  for (uint32_t i = 0; i < 3; ++i) {
    bindings[i].binding = i;
    bindings[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[i].descriptorCount = 1;
    bindings[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
  }

  VkDescriptorSetLayoutCreateInfo layoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
  layoutInfo.bindingCount = 3;
  layoutInfo.pBindings = bindings;

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};

  VkComputePipelineCreateInfo pipelineInfo = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
  pipelineInfo.stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  pipelineInfo.stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  pipelineInfo.stage.module = shaderModule;
  pipelineInfo.stage.pName = "main";

  bool ok = true;

  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &conv.descSetLayout) != VK_SUCCESS) {
    REC_LOG("[NV12] ERROR: vkCreateDescriptorSetLayout failed");
    ok = false;
  }

  if (ok) {
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &conv.descSetLayout;
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &conv.pipelineLayout) != VK_SUCCESS) {
      REC_LOG("[NV12] ERROR: vkCreatePipelineLayout failed");
      ok = false;
    } else {
      pipelineInfo.layout = conv.pipelineLayout;
      if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &conv.pipeline) != VK_SUCCESS) {
        REC_LOG("[NV12] ERROR: vkCreateComputePipelines failed");
        ok = false;
      }
    }
  }

  if (ok) {
    VkDescriptorPoolSize poolSize = {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3};
    VkDescriptorPoolCreateInfo descPoolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    descPoolInfo.maxSets = 1;
    descPoolInfo.poolSizeCount = 1;
    descPoolInfo.pPoolSizes = &poolSize;

    if (vkCreateDescriptorPool(device, &descPoolInfo, nullptr, &conv.descPool) != VK_SUCCESS) {
      REC_LOG("[NV12] ERROR: vkCreateDescriptorPool failed");
      ok = false;
    } else {
      VkDescriptorSetAllocateInfo setInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
      setInfo.descriptorPool = conv.descPool;
      setInfo.descriptorSetCount = 1;
      setInfo.pSetLayouts = &conv.descSetLayout;
      if (vkAllocateDescriptorSets(device, &setInfo, &conv.descSet) != VK_SUCCESS) {
        REC_LOG("[NV12] ERROR: vkAllocateDescriptorSets failed");
        ok = false;
      }
    }
  }

  vkDestroyShaderModule(device, shaderModule, nullptr);

  if (!ok) {
    conv.disabled = true;
    return false;
  }

  conv.initialized = true;
  return true;
}

void VulkanCapture::DestroyIntermediateImage() {
  if (conv.intermediateView != VK_NULL_HANDLE) {
    vkDestroyImageView(device, conv.intermediateView, nullptr);
    conv.intermediateView = VK_NULL_HANDLE;
  }
  if (conv.intermediateImage != VK_NULL_HANDLE) {
    vkDestroyImage(device, conv.intermediateImage, nullptr);
    conv.intermediateImage = VK_NULL_HANDLE;
  }
  if (conv.intermediateMemory != VK_NULL_HANDLE) {
    vkFreeMemory(device, conv.intermediateMemory, nullptr);
    conv.intermediateMemory = VK_NULL_HANDLE;
  }
  conv.intermediateInTransferDst = false;
  conv.width = 0;
  conv.height = 0;
}

void VulkanCapture::DestroyConverter() {
  if (device) {
    DestroyDstPlaneViews();
    DestroyIntermediateImage();
    if (conv.pipeline) vkDestroyPipeline(device, conv.pipeline, nullptr);
    if (conv.pipelineLayout) vkDestroyPipelineLayout(device, conv.pipelineLayout, nullptr);
    if (conv.descPool) vkDestroyDescriptorPool(device, conv.descPool, nullptr);  // frees descSet
    if (conv.descSetLayout) vkDestroyDescriptorSetLayout(device, conv.descSetLayout, nullptr);
    if (conv.captureFence) vkDestroyFence(device, conv.captureFence, nullptr);
    if (conv.cmdPool) vkDestroyCommandPool(device, conv.cmdPool, nullptr);  // frees cmdBuf
  }
  if (conv.pendingFrame) av_frame_free(&conv.pendingFrame);
  conv = Converter{};
}

bool VulkanCapture::EnsureIntermediateImage(uint32_t w, uint32_t h) {
  if (conv.intermediateImage != VK_NULL_HANDLE && conv.width == w && conv.height == h) return true;

  DestroyIntermediateImage();

  VkImageCreateInfo imageInfo = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
  imageInfo.imageType = VK_IMAGE_TYPE_2D;
  imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  imageInfo.extent.width = w;
  imageInfo.extent.height = h;
  imageInfo.extent.depth = 1;
  imageInfo.mipLevels = 1;
  imageInfo.arrayLayers = 1;
  imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
  imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
  imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

  if (vkCreateImage(device, &imageInfo, nullptr, &conv.intermediateImage) != VK_SUCCESS) {
    REC_LOG("[NV12] ERROR: vkCreateImage failed for %ux%u", w, h);
    conv.intermediateImage = VK_NULL_HANDLE;
    conv.disabled = true;
    return false;
  }

  VkMemoryRequirements requirements = {};
  vkGetImageMemoryRequirements(device, conv.intermediateImage, &requirements);

  VkMemoryAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
  allocInfo.allocationSize = requirements.size;
  allocInfo.memoryTypeIndex = FindMemoryType(memoryProperties, requirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &conv.intermediateMemory) != VK_SUCCESS) {
    REC_LOG("[NV12] ERROR: vkAllocateMemory failed for intermediate image");
    DestroyIntermediateImage();
    conv.disabled = true;
    return false;
  }

  if (vkBindImageMemory(device, conv.intermediateImage, conv.intermediateMemory, 0) != VK_SUCCESS) {
    REC_LOG("[NV12] ERROR: vkBindImageMemory failed for intermediate image");
    DestroyIntermediateImage();
    conv.disabled = true;
    return false;
  }

  VkImageViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  viewInfo.image = conv.intermediateImage;
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

  if (vkCreateImageView(device, &viewInfo, nullptr, &conv.intermediateView) != VK_SUCCESS) {
    REC_LOG("[NV12] ERROR: vkCreateImageView failed for intermediate image");
    DestroyIntermediateImage();
    conv.disabled = true;
    return false;
  }

  conv.width = w;
  conv.height = h;
  return true;
}

VulkanCapture::PlaneViews VulkanCapture::GetDstPlaneViews(const AVVkFrame* vkFrame) {
  auto cached = conv.dstViewCache.find(vkFrame->img[0]);
  if (cached != conv.dstViewCache.end()) return cached->second;

  PlaneViews views{};

  const bool multiplane = (vkFrame->img[1] == VK_NULL_HANDLE);

  VkImageViewCreateInfo viewInfo = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
  viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};

  viewInfo.image = vkFrame->img[0];
  viewInfo.format = VK_FORMAT_R8_UNORM;
  viewInfo.subresourceRange.aspectMask = multiplane ? VK_IMAGE_ASPECT_PLANE_0_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
  if (vkCreateImageView(device, &viewInfo, nullptr, &views.y) != VK_SUCCESS) {
    REC_LOG("[NV12] ERROR: failed to create Y plane view (multiplane=%d)", (int)multiplane);
    views.y = VK_NULL_HANDLE;
    return views;
  }

  if (multiplane) {
    viewInfo.image = vkFrame->img[0];
    viewInfo.format = VK_FORMAT_R8G8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_PLANE_1_BIT;
  } else {
    viewInfo.image = vkFrame->img[1];
    viewInfo.format = VK_FORMAT_R8G8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  }

  if (vkCreateImageView(device, &viewInfo, nullptr, &views.uv) != VK_SUCCESS) {
    REC_LOG("[NV12] ERROR: failed to create UV plane view (multiplane=%d)", (int)multiplane);
    vkDestroyImageView(device, views.y, nullptr);
    views.y = VK_NULL_HANDLE;
    views.uv = VK_NULL_HANDLE;
    return views;
  }

  conv.dstViewCache[vkFrame->img[0]] = views;
  return views;
}

void VulkanCapture::DestroyDstPlaneViews() {
  if (device) {
    for (auto& entry : conv.dstViewCache) {
      if (entry.second.y) vkDestroyImageView(device, entry.second.y, nullptr);
      if (entry.second.uv) vkDestroyImageView(device, entry.second.uv, nullptr);
    }
  }
  conv.dstViewCache.clear();
}

void VulkanCapture::ConvertFrameToNV12(VkQueue queue, VkImage srcImage, AVVkFrame* vkFrame, const VkPresentInfoKHR* present, VkSemaphore signalSemaphore) {
  VkCommandBuffer cmdBuf = conv.cmdBuf;
  const bool multiplane = (vkFrame->img[1] == VK_NULL_HANDLE);
  const uint32_t w = width;
  const uint32_t h = height;

  VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(cmdBuf, &beginInfo);

  {
    VkImageMemoryBarrier barriers[2] = {};
    uint32_t count = 0;

    barriers[count].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barriers[count].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    barriers[count].dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barriers[count].oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barriers[count].newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barriers[count].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[count].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[count].image = srcImage;
    barriers[count].subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    ++count;

    if (!conv.intermediateInTransferDst) {
      barriers[count].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      barriers[count].srcAccessMask = 0;
      barriers[count].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
      barriers[count].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      barriers[count].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
      barriers[count].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barriers[count].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barriers[count].image = conv.intermediateImage;
      barriers[count].subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
      ++count;
      conv.intermediateInTransferDst = true;
    }

    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, count, barriers);
  }

  VkImageBlit blitRegion = {};
  blitRegion.srcSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
  blitRegion.srcOffsets[1] = {(int32_t)w, (int32_t)h, 1};
  blitRegion.dstSubresource = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1};
  blitRegion.dstOffsets[1] = {(int32_t)w, (int32_t)h, 1};

  vkCmdBlitImage(cmdBuf, srcImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, conv.intermediateImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitRegion, VK_FILTER_NEAREST);

  {
    VkImageMemoryBarrier barriers[3] = {};
    uint32_t count = 0;

    barriers[count].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barriers[count].srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barriers[count].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barriers[count].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barriers[count].newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barriers[count].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[count].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[count].image = conv.intermediateImage;
    barriers[count].subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    ++count;

    const VkImageAspectFlags dstAspect = multiplane ? static_cast<VkImageAspectFlags>(VK_IMAGE_ASPECT_PLANE_0_BIT | VK_IMAGE_ASPECT_PLANE_1_BIT) : static_cast<VkImageAspectFlags>(VK_IMAGE_ASPECT_COLOR_BIT);

    const uint32_t planeCount = multiplane ? 1 : 2;
    for (uint32_t i = 0; i < planeCount; ++i) {
      barriers[count].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      barriers[count].srcAccessMask = 0;
      barriers[count].dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
      barriers[count].oldLayout = vkFrame->layout[i];
      barriers[count].newLayout = VK_IMAGE_LAYOUT_GENERAL;
      barriers[count].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barriers[count].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barriers[count].image = vkFrame->img[i];
      barriers[count].subresourceRange = {dstAspect, 0, 1, 0, 1};
      ++count;
    }

    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, 0, 0, nullptr, 0, nullptr, count, barriers);
  }

  vkCmdBindPipeline(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, conv.pipeline);
  vkCmdBindDescriptorSets(cmdBuf, VK_PIPELINE_BIND_POINT_COMPUTE, conv.pipelineLayout, 0, 1, &conv.descSet, 0, nullptr);
  vkCmdDispatch(cmdBuf, (w + 7) / 8, (h + 7) / 8, 1);

  {
    VkImageMemoryBarrier barriers[2] = {};
    uint32_t count = 0;

    barriers[count].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barriers[count].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    barriers[count].dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barriers[count].oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    barriers[count].newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barriers[count].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[count].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[count].image = conv.intermediateImage;
    barriers[count].subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    ++count;

    barriers[count].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barriers[count].srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barriers[count].dstAccessMask = 0;
    barriers[count].oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barriers[count].newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    barriers[count].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[count].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barriers[count].image = srcImage;
    barriers[count].subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    ++count;

    vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, count, barriers);
  }

  vkEndCommandBuffer(cmdBuf);

  vkResetFences(device, 1, &conv.captureFence);

  std::vector<VkPipelineStageFlags> waitStages(present->waitSemaphoreCount, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);

  VkSubmitInfo submitInfo = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
  submitInfo.waitSemaphoreCount = present->waitSemaphoreCount;
  submitInfo.pWaitSemaphores = present->pWaitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages.data();
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cmdBuf;
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = &signalSemaphore;
  vkQueueSubmit(queue, 1, &submitInfo, conv.captureFence);

  const uint32_t planeCount = multiplane ? 1 : 2;
  for (uint32_t i = 0; i < planeCount; ++i) vkFrame->layout[i] = VK_IMAGE_LAYOUT_GENERAL;
}

bool VulkanCapture::CaptureFrameLocked(VkQueue queue, uint32_t imageIndex, const VkPresentInfoKHR* present, VkSemaphore* outSemaphore) {
  if (imageIndex >= swapchainImages.size() || imageIndex >= presentSemaphores.size()) return false;

  if (!EnsureEncoder(width, height) || !InitConverter() || !EnsureIntermediateImage(width, height)) {
    REC_LOG("ERROR: Capture failed, stopping recording");
    StopLocked();
    return false;
  }

  FlushPendingFrame();

  AVFrame* frame = av_frame_alloc();
  if (!frame) return false;

  frame->format = AV_PIX_FMT_VULKAN;
  frame->width = width;
  frame->height = height;

  if (av_hwframe_get_buffer(encoderCtx->hw_frames_ctx, frame, 0) < 0) {
    av_frame_free(&frame);
    return false;
  }

  AVVkFrame* vkFrame = (AVVkFrame*)frame->data[0];

  PlaneViews views = GetDstPlaneViews(vkFrame);
  if (!views.y || !views.uv) {
    av_frame_free(&frame);
    return false;
  }

  {
    VkDescriptorImageInfo infos[3] = {};
    infos[0].imageView = conv.intermediateView;
    infos[0].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    infos[1].imageView = views.y;
    infos[1].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    infos[2].imageView = views.uv;
    infos[2].imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet writes[3] = {};
    for (uint32_t i = 0; i < 3; ++i) {
      writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writes[i].dstSet = conv.descSet;
      writes[i].dstBinding = i;
      writes[i].descriptorCount = 1;
      writes[i].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      writes[i].pImageInfo = &infos[i];
    }
    vkUpdateDescriptorSets(device, 3, writes, 0, nullptr);
  }

  VkSemaphore signalSemaphore = presentSemaphores[imageIndex];
  ConvertFrameToNV12(queue, swapchainImages[imageIndex], vkFrame, present, signalSemaphore);
  *outSemaphore = signalSemaphore;

  if (!captureStartValid) {
    captureStart = std::chrono::steady_clock::now();
    captureStartValid = true;
  }

  int64_t pts;
  if (fps > 0) {
    pts = frameIndex * (kCaptureTimeScale / fps);
  } else {
    const auto elapsedUs = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - captureStart).count();
    pts = (int64_t)(elapsedUs * kCaptureTimeScale / 1000000);
    if (pts <= lastPts) pts = lastPts + 1;
  }
  ++frameIndex;
  lastPts = pts;

  conv.pendingFrame = frame;
  conv.pendingPts = pts;
  conv.pendingDuration = fps > 0 ? (kCaptureTimeScale / fps) : (kCaptureTimeScale / 60);
  return true;
}
