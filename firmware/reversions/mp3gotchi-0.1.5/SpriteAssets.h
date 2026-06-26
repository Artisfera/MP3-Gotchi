#pragma once
#include <Arduino.h>
#include <pgmspace.h>

struct AssetPixel { uint8_t x; uint8_t y; uint16_t color; };
struct SparseAsset { uint8_t w; uint8_t h; uint16_t count; const AssetPixel* pixels; };

extern const SparseAsset ASSET_MONKEY_BG;
extern const SparseAsset ASSET_EYES_OPEN;
extern const SparseAsset ASSET_EYES_DIZZY_1;
extern const SparseAsset ASSET_EYES_DIZZY_2;
extern const SparseAsset ASSET_MOUTH_SMILE;
extern const SparseAsset ASSET_MOUTH_DIZZY_1;
extern const SparseAsset ASSET_MOUTH_DIZZY_2;
extern const SparseAsset ASSET_NOSE;
extern const SparseAsset ASSET_ICON_HEADPHONES;
extern const SparseAsset ASSET_ICON_SDCARD;
extern const SparseAsset ASSET_ICON_BATTERY;
extern const SparseAsset ASSET_SONG_TITLE_AREA;

