#pragma once

#include <stdint.h>

constexpr size_t MAX_POINT_LIGHTS = 512U;
constexpr size_t MAX_SPOT_LIGHTS = 512U;
constexpr size_t MAX_SPRITES = 2048U;
constexpr size_t MAX_WSPRITES = 1024U;
constexpr size_t MAX_DECALS = 512U;

constexpr size_t MAX_SUBMESH_COUNT = 16U;
constexpr size_t MAX_SORTED_MESHES = 256U;
constexpr size_t MAX_UNIQUE_MESHES = 128U;
constexpr size_t MAX_ANIM_SORTED_MESHES = 64U;
constexpr size_t MAX_ANIM_UNIQUE_MESHES = 128U;

constexpr unsigned int NUMB_MODELSPERTYPE = 256;
constexpr unsigned int NUMB_ANIMMODELSPERTYPE = 128;

constexpr unsigned short MAX_BONECOUNT = 64;


constexpr uint32_t STEP = 60U;
constexpr float STEPF = float(STEP);
constexpr float INV_STEPF = 1.0f / STEPF;

constexpr unsigned short NUMBOF_RENDERTARGETS = 8;
constexpr unsigned short NUMBOF_REFLECTIONPROBES = 16;
constexpr unsigned short MAX_NUMBOF_REFLECTIONPROBES = NUMBOF_REFLECTIONPROBES * 2;
constexpr unsigned short NUMBOF_SHGRIDS = 16;
constexpr unsigned short MAX_NUMBOF_SHGRIDS = NUMBOF_SHGRIDS * 2;
constexpr unsigned short MAX_TEXTURE_AMOUNT = 12;

constexpr unsigned short CLUSTER_WIDTH = 16;
constexpr unsigned short CLUSTER_HEIGTH = 8;
constexpr unsigned short CLUSTER_DEPTH = 8;
constexpr unsigned short CLUSTER_WIDTH_GPU = 16;
constexpr unsigned short CLUSTER_HEIGTH_GPU = 9;
constexpr unsigned short CLUSTER_DEPTH_GPU = 24;
constexpr unsigned short NOISE_HALFHEMISPHEREAMOUNT = 64;
constexpr unsigned short NOISE_ROTATIONALTEXTURESIZE = 4;

constexpr unsigned int SHADOWMAPDIRECTIONAL_SIZE = 4096;
constexpr unsigned int SHADOWMAP_SIZE = 8192;
constexpr unsigned int SHADOWMAP_STATIC_SIZE = 4096;
constexpr unsigned int SHADOWMAP_TILESIZE = 128;
constexpr unsigned int NUMB_SHADOWMAP_TILES = SHADOWMAP_SIZE / SHADOWMAP_TILESIZE;
constexpr unsigned int NUMB_STATICSHADOWMAP_TILES = NUMB_SHADOWMAP_TILES / 2;
constexpr unsigned int NUMB_SHADOWMAP_TILETOTAL = (SHADOWMAP_SIZE / SHADOWMAP_TILESIZE) * (SHADOWMAP_SIZE / SHADOWMAP_TILESIZE);

constexpr float LIGHTRANGE_THRESHOLD1 = 1000.f * 1000.f;
constexpr float LIGHTRANGE_THRESHOLD2 = 2500.f * 2500.f;
constexpr float LIGHTRANGE_THRESHOLD3 = 6000.f * 6000.f;

constexpr unsigned short NUMBOF_CHARACTERPRESETS = 5;
constexpr unsigned short NUMBOF_CHARACTERACCESORIES = 20;
constexpr unsigned short NUMBOF_MAXAMOUNTOFPLAYERS = 16;
constexpr unsigned short NUMBOF_MAXAMOUNTOFRRENDERDATAPACKAGES = 32;

constexpr unsigned short CUBEMAP_TEXTURESIZE = 128;

constexpr unsigned int NUMB_PARTICLES_PERSYSTEM = 48;
constexpr unsigned int MAX_COUNTMULTIEMITTER = 3;
constexpr unsigned int MAX_PARTICLES = 256;

namespace Engine { struct RenderData; }