#include "Resolution.h"

#include <cdll_int.h>
#include <convar.h>
#include <modes.h>

#include "base/Interfaces.h"

#define MAX_MODE_LIST 512

// https://github.com/jooonior/SourceRes
namespace SourceRes {

// Find the address of the engine's internal video mode count.
int* GetInternalModeCount(const vmode_s* modeList, int count) {
  // Mode count should be right before the mode array.
  int* modeCount = (int*)modeList - 1;

  // Verify that the value matches.
  if (*modeCount != count) {
    throw "Failed to access video mode count. Aborting.";
  }

  return modeCount;
}

/**
 * Register a new video mode with specific resolution.
 * Does nothing if a mode with the same resolution is already present.
 *
 * @return If a new video mode was added.
 */
static bool RegisterResolution(int width, int height) {
  // Get modes.
  vmode_s* modeList;
  int count;
  Interfaces::EngineClient->GetVideoModes(count, modeList);

  // Return if a mode with desired resolution is already present.
  for (auto i = 0; i < count; i++) {
    vmode_s mode = modeList[i];
    if (mode.width == width && mode.height == height) {
      return false;
    }
  }

  if (count >= MAX_MODE_LIST) {
    throw "Video mode array is full. Can't add any more modes.";
  }

  int* modeCount = SourceRes::GetInternalModeCount(modeList, count);

  // These seem to have the same value across all modes.
  int refreshRate = modeList[0].refreshRate;
  int bitsPerPixel = modeList[0].bpp;

  // Shift modes with higher resolution.
  for (; count; count--) {
    vmode_s mode = modeList[count - 1];
    if (mode.width < width || (mode.width == width && mode.height < height)) break;

    modeList[count] = modeList[count - 1];
  }

  // Insert new mode.
  modeList[count].width = width;
  modeList[count].height = height;
  modeList[count].bpp = bitsPerPixel;
  modeList[count].refreshRate = refreshRate;

  // Update the internal counter.
  *modeCount += 1;

  return true;
}

}  // namespace SourceRes

void Resolution::resolution_set(const CCommand& args) {
  if (args.ArgC() != 3) {
    return ConMsg("Usage:  resolution_set <width> <height>\n");
  }

  int width = strtol(args.Arg(1), NULL, 10);
  int height = strtol(args.Arg(2), NULL, 10);

  SourceRes::RegisterResolution(width, height);

  char cmd[32];
  sprintf(cmd, "mat_setvideomode %i %i 1", width, height);
  Interfaces::EngineClient->ClientCmd_Unrestricted(cmd);
}