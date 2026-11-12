#pragma once

#include <vulkan/vulkan.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "base/Hook.h"

struct AVCodecContext;
struct AVBufferRef;
struct AVFormatContext;
struct AVStream;
struct AVFrame;
struct AVPacket;
struct AVVkFrame;

class VulkanCapture {
 public:
  void Load();
  void Unload();

  bool Start(const char* filename, int fps = 0);
  void Stop();

  bool IsRecording() const { return recording.load(); }

 private:
  // fake device to hook functions
  VkDevice fakeDevice = VK_NULL_HANDLE;
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkAllocationCallbacks* allocator = NULL;
  VkInstance instance = VK_NULL_HANDLE;

  uint32_t queueFamily = (uint32_t)-1;
  std::vector<VkQueueFamilyProperties> queueFamilies;

  VkPhysicalDeviceMemoryProperties memoryProperties{};
  bool rgba8StorageSupported = false;

  bool CreateFakeDevice();
  void DestroyFakeDevice();

 private:
  // real device, filled in CreateSwapchainKHR
  VkDevice device = VK_NULL_HANDLE;
  VkSwapchainKHR swapchain = VK_NULL_HANDLE;
  VkFormat swapchainFormat = VK_FORMAT_UNDEFINED;
  uint32_t width = 0;
  uint32_t height = 0;
  std::vector<VkImage> swapchainImages;

  std::vector<VkSemaphore> presentSemaphores;
  std::vector<VkSemaphore> retiredSemaphores;

  std::mutex mutex;
  std::atomic<bool> recording{false};
  std::string basePath;
  int fps = 0;

  AVBufferRef* hwDeviceCtx = nullptr;
  AVCodecContext* encoderCtx = nullptr;
  AVFormatContext* formatCtx = nullptr;
  AVStream* videoStream = nullptr;
  AVPacket* packet = nullptr;
  bool encoderReady = false;
  uint32_t encoderWidth = 0;
  uint32_t encoderHeight = 0;

  int64_t frameIndex = 0;
  int64_t lastPts = -1;
  std::chrono::steady_clock::time_point captureStart;
  bool captureStartValid = false;

  bool StartLocked(const char* filename, int fps);
  void StopLocked();

  bool EnsurePresentSemaphoresLocked();
  bool CaptureFrameLocked(VkQueue queue, uint32_t imageIndex, const VkPresentInfoKHR* present, VkSemaphore* outSemaphore);

  // encoder / muxer
  bool EnsureEncoder(uint32_t w, uint32_t h);
  bool OpenMuxer();
  void CloseMuxer();
  int WritePackets();
  void FlushPendingFrame();
  void CloseEncoderLocked();

  // GPU BGRA -> NV12 converter
  struct PlaneViews {
    VkImageView y = VK_NULL_HANDLE;
    VkImageView uv = VK_NULL_HANDLE;
  };

  struct Converter {
    bool initialized = false;
    bool disabled = false;

    VkCommandPool cmdPool = VK_NULL_HANDLE;
    VkCommandBuffer cmdBuf = VK_NULL_HANDLE;
    VkFence captureFence = VK_NULL_HANDLE;

    AVFrame* pendingFrame = nullptr;
    int64_t pendingPts = 0;
    int64_t pendingDuration = 0;

    VkDescriptorSetLayout descSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkDescriptorPool descPool = VK_NULL_HANDLE;
    VkDescriptorSet descSet = VK_NULL_HANDLE;

    VkImage intermediateImage = VK_NULL_HANDLE;
    VkDeviceMemory intermediateMemory = VK_NULL_HANDLE;
    VkImageView intermediateView = VK_NULL_HANDLE;
    uint32_t width = 0;
    uint32_t height = 0;
    bool intermediateInTransferDst = false;

    std::unordered_map<VkImage, PlaneViews> dstViewCache;
  } conv;

  bool InitConverter();
  void DestroyConverter();
  void DestroyIntermediateImage();
  bool EnsureIntermediateImage(uint32_t w, uint32_t h);
  PlaneViews GetDstPlaneViews(const AVVkFrame* vkFrame);
  void DestroyDstPlaneViews();
  void ConvertFrameToNV12(VkQueue queue, VkImage srcImage, AVVkFrame* vkFrame, const VkPresentInfoKHR* present, VkSemaphore signalSemaphore);

  bool ffmpegAvailable = false;

 private:
  // called at end of every game frame
  VkResult VKAPI_CALL QueuePresentKHR(VkQueue queue, const VkPresentInfoKHR* pPresentInfo);
  Hook hookQueuePresentKHR;

  // called on resolution change
  VkResult VKAPI_CALL CreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSwapchainKHR* pSwapchain);
  Hook hookCreateSwapchainKHR;
};
