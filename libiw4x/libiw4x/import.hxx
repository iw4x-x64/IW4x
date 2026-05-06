#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
#  ifndef NOMINMAX
#    define NOMINMAX
#    include <winsock2.h>
#    include <d3d9.h>
#    undef NOMINMAX
#  else
#    include <winsock2.h>
#    include <d3d9.h>
#  endif
#  undef WIN32_LEAN_AND_MEAN
#else
#  ifndef NOMINMAX
#    define NOMINMAX
#    include <winsock2.h>
#    include <d3d9.h>
#    undef NOMINMAX
#  else
#    include <winsock2.h>
#    include <d3d9.h>
#  endif
#endif

#include <cstddef>
#include <cstdint>

namespace iw4x
{
  struct expressionEntry;
  struct ExpressionSupportingData;
  struct FxElemDef;
  struct GfxPortal;
  struct itemDef_s;
  struct MaterialTechnique;
  struct MenuEventHandler;
  struct pathnode_t;
  struct pathnode_tree_t;
  struct Statement_s;

  typedef float vec_t;

  // ???
  //
  union vec2_t
  {
    float v [2];

    // ???
    //
    struct
    {
      float x;
      float y;
    };
  };

  // ???
  //
  union vec3_t
  {
    float v [3];

    // ???
    //
    struct
    {
      float x;
      float y;
      float z;
    };
  };

  // ???
  //
  union vec4_t
  {
    float value [4];

    // ???
    //
    struct
    {
      float x;
      float y;
      float z;
      float w;
    }; // s1;

    // ???
    //
    struct
    {
      float r;
      float g;
      float b;
      float a;
    }; // s2;
  };

  // ???
  //
  enum DvarFlags
  {
    DVAR_NONE = 0x0,
    DVAR_ARCHIVE = 0x1,
    DVAR_LATCH = 0x2,
    DVAR_CHEAT = 0x4,
    DVAR_CODINFO = 0x8,
    DVAR_SCRIPTINFO = 0x10,
    DVAR_TEMP = 0x20,
    DVAR_SAVED = 0x40,
    DVAR_INTERNAL = 0x80,
    DVAR_EXTERNAL = 0x100,
    DVAR_USERINFO = 0x200,
    DVAR_SERVERINFO = 0x400,
    DVAR_INIT = 0x800,
    DVAR_SYSTEMINFO = 0x1000,
    DVAR_ROM = 0x2000,
    DVAR_CHANGEABLE_RESET = 0x4000,
    DVAR_AUTOEXEC = 0x8000,
  };

  // 15
  //
  enum trType_t
  {
    TR_STATIONARY = 0x0,
    TR_INTERPOLATE = 0x1,
    TR_LINEAR = 0x2,
    TR_LINEAR_STOP = 0x3,
    TR_SINE = 0x4,
    TR_GRAVITY = 0x5,
    TR_LOW_GRAVITY = 0x6,
    TR_ACCELERATE = 0x7,
    TR_DECELERATE = 0x8,
    TR_PHYSICS = 0x9,
    TR_FIRST_RAGDOLL = 0xA,
    TR_RAGDOLL = 0xA,
    TR_RAGDOLL_GRAVITY = 0xB,
    TR_RAGDOLL_INTERPOLATE = 0xC,
    TR_LAST_RAGDOLL = 0xC,
    NUM_TRTYPES = 0xD,
  };

  // 23
  //
  enum MissileFlightMode
  {
    MISSILEFLIGHTMODE_TOP = 0x0,
    MISSILEFLIGHTMODE_DIRECT = 0x1,
    MISSILEFLIGHTMODE_COUNT = 0x2,
  };

  // 25
  //
  enum ViewLockTypes
  {
    PLAYERVIEWLOCK_NONE = 0x0,
    PLAYERVIEWLOCK_FULL = 0x1,
    PLAYERVIEWLOCK_WEAPONJITTER = 0x2,
    PLAYERVIEWLOCKCOUNT = 0x3,
  };

  // 29
  //
  enum netsrc_t
  {
    NS_CLIENT1 = 0x0,
    NS_SERVER = 0x1,
    NS_MAXCLIENTS = 0x1,
    NS_PACKET = 0x2,
  };

  // 38
  //
  enum XAssetType
  {
    ASSET_TYPE_PHYSPRESET = 0x0,
    ASSET_TYPE_PHYSCOLLMAP = 0x1,
    ASSET_TYPE_XANIMPARTS = 0x2,
    ASSET_TYPE_XMODEL_SURFS = 0x3,
    ASSET_TYPE_XMODEL = 0x4,
    ASSET_TYPE_MATERIAL = 0x5,
    ASSET_TYPE_PIXELSHADER = 0x6,
    ASSET_TYPE_VERTEXSHADER = 0x7,
    ASSET_TYPE_VERTEXDECL = 0x8,
    ASSET_TYPE_TECHNIQUE_SET = 0x9,
    ASSET_TYPE_IMAGE = 0xA,
    ASSET_TYPE_SOUND = 0xB,
    ASSET_TYPE_SOUND_CURVE = 0xC,
    ASSET_TYPE_LOADED_SOUND = 0xD,
    ASSET_TYPE_CLIPMAP_SP = 0xE,
    ASSET_TYPE_CLIPMAP_MP = 0xF,
    ASSET_TYPE_COMWORLD = 0x10,
    ASSET_TYPE_GAMEWORLD_SP = 0x11,
    ASSET_TYPE_GAMEWORLD_MP = 0x12,
    ASSET_TYPE_MAP_ENTS = 0x13,
    ASSET_TYPE_FXWORLD = 0x14,
    ASSET_TYPE_GFXWORLD = 0x15,
    ASSET_TYPE_LIGHT_DEF = 0x16,
    ASSET_TYPE_UI_MAP = 0x17,
    ASSET_TYPE_FONT = 0x18,
    ASSET_TYPE_MENULIST = 0x19,
    ASSET_TYPE_MENU = 0x1A,
    ASSET_TYPE_LOCALIZE_ENTRY = 0x1B,
    ASSET_TYPE_WEAPON = 0x1C,
    ASSET_TYPE_SNDDRIVER_GLOBALS = 0x1D,
    ASSET_TYPE_FX = 0x1E,
    ASSET_TYPE_IMPACT_FX = 0x1F,
    ASSET_TYPE_AITYPE = 0x20,
    ASSET_TYPE_MPTYPE = 0x21,
    ASSET_TYPE_CHARACTER = 0x22,
    ASSET_TYPE_XMODELALIAS = 0x23,
    ASSET_TYPE_RAWFILE = 0x24,
    ASSET_TYPE_STRINGTABLE = 0x25,
    ASSET_TYPE_LEADERBOARD = 0x26,
    ASSET_TYPE_STRUCTURED_DATA_DEF = 0x27,
    ASSET_TYPE_TRACER = 0x28,
    ASSET_TYPE_VEHICLE = 0x29,
    ASSET_TYPE_ADDON_MAP_ENTS = 0x2A,
    ASSET_TYPE_COUNT = 0x2B,
    ASSET_TYPE_STRING = 0x2B,
    ASSET_TYPE_ASSETLIST = 0x2C,
  };

  // 39
  //
  enum errorParm_t
  {
    ERR_FATAL = 0x0,
    ERR_DROP = 0x1,
    ERR_SERVERDISCONNECT = 0x2,
    ERR_DISCONNECT = 0x3,
    ERR_SCRIPT = 0x4,
    ERR_SCRIPT_DROP = 0x5,
    ERR_LOCALIZATION = 0x6,
    ERR_MAPLOADERRORSUMMARY = 0x7,
  };

  // 61
  //
  enum PlayerHandIndex
  {
    WEAPON_HAND_RIGHT = 0x0,
    WEAPON_HAND_LEFT = 0x1,
    NUM_WEAPON_HANDS = 0x2,
    WEAPON_HAND_DEFAULT = 0x0,
  };


  // 65
  //
  enum ActionSlotType
  {
    ACTIONSLOTTYPE_DONOTHING = 0x0,
    ACTIONSLOTTYPE_SPECIFYWEAPON = 0x1,
    ACTIONSLOTTYPE_ALTWEAPONTOGGLE = 0x2,
    ACTIONSLOTTYPE_NIGHTVISION = 0x3,
    ACTIONSLOTTYPECOUNT = 0x4,
  };

  // 68
  //
  enum clientMigState_t
  {
    CMSTATE_INACTIVE = 0x0,
    CMSTATE_OLDHOSTLEAVING = 0x1,
    CMSTATE_LIMBO = 0x2,
    CMSTATE_NEWHOSTCONNECT = 0x3,
    CMSTATE_COUNT = 0x4,
  };

  // 77
  //
  enum weapFireType_t
  {
    WEAPON_FIRETYPE_FULLAUTO = 0x0,
    WEAPON_FIRETYPE_SINGLESHOT = 0x1,
    WEAPON_FIRETYPE_BURSTFIRE2 = 0x2,
    WEAPON_FIRETYPE_BURSTFIRE3 = 0x3,
    WEAPON_FIRETYPE_BURSTFIRE4 = 0x4,
    WEAPON_FIRETYPE_DOUBLEBARREL = 0x5,
    WEAPON_FIRETYPECOUNT = 0x6,
    WEAPON_FIRETYPE_BURSTFIRE_FIRST = 0x2,
    WEAPON_FIRETYPE_BURSTFIRE_LAST = 0x4,
  };

  // 79
  //
  enum OffhandClass
  {
    OFFHAND_CLASS_NONE = 0x0,
    OFFHAND_CLASS_FRAG_GRENADE = 0x1,
    OFFHAND_CLASS_SMOKE_GRENADE = 0x2,
    OFFHAND_CLASS_FLASH_GRENADE = 0x3,
    OFFHAND_CLASS_THROWINGKNIFE = 0x4,
    OFFHAND_CLASS_OTHER = 0x5,
    OFFHAND_CLASS_COUNT = 0x6,
  };

  // 82
  //
  enum connstate_t
  {
    CA_DISCONNECTED = 0x0,
    CA_CINEMATIC = 0x1,
    CA_LOGO = 0x2,
    CA_CONNECTING = 0x3,
    CA_CHALLENGING = 0x4,
    CA_CONNECTED = 0x5,
    CA_SENDINGSTATS = 0x6,
    CA_LOADING = 0x7,
    CA_PRIMED = 0x8,
    CA_ACTIVE = 0x9,
  };

  // 83
  //
  enum WeapStickinessType
  {
    WEAPSTICKINESS_NONE = 0x0,
    WEAPSTICKINESS_ALL = 0x1,
    WEAPSTICKINESS_ALL_ORIENT = 0x2,
    WEAPSTICKINESS_GROUND = 0x3,
    WEAPSTICKINESS_GROUND_WITH_YAW = 0x4,
    WEAPSTICKINESS_KNIFE = 0x5,
    WEAPSTICKINESS_COUNT = 0x6,
  };

  // 92
  //
  enum keyNum_t
  {
    K_NONE = 0x0,
    K_TAB = 0x9,
    K_ENTER = 0xD,
    K_ESCAPE = 0x1B,
    K_SPACE = 0x20,
    K_BACKSPACE = 0x7F,
    K_ASCII_FIRST = 0x80,
    K_ASCII_181 = 0x80,
    K_ASCII_191 = 0x81,
    K_ASCII_223 = 0x82,
    K_ASCII_224 = 0x83,
    K_ASCII_225 = 0x84,
    K_ASCII_228 = 0x85,
    K_ASCII_229 = 0x86,
    K_ASCII_230 = 0x87,
    K_ASCII_231 = 0x88,
    K_ASCII_232 = 0x89,
    K_ASCII_233 = 0x8A,
    K_ASCII_236 = 0x8B,
    K_ASCII_241 = 0x8C,
    K_ASCII_242 = 0x8D,
    K_ASCII_243 = 0x8E,
    K_ASCII_246 = 0x8F,
    K_ASCII_248 = 0x90,
    K_ASCII_249 = 0x91,
    K_ASCII_250 = 0x92,
    K_ASCII_252 = 0x93,
    K_END_ASCII_CHARS = 0x94,
    K_COMMAND = 0x96,
    K_CAPSLOCK = 0x97,
    K_POWER = 0x98,
    K_PAUSE = 0x99,
    K_UPARROW = 0x9A,
    K_DOWNARROW = 0x9B,
    K_LEFTARROW = 0x9C,
    K_RIGHTARROW = 0x9D,
    K_ALT = 0x9E,
    K_CTRL = 0x9F,
    K_SHIFT = 0xA0,
    K_INS = 0xA1,
    K_DEL = 0xA2,
    K_PGDN = 0xA3,
    K_PGUP = 0xA4,
    K_HOME = 0xA5,
    K_END = 0xA6,
    K_F1 = 0xA7,
    K_F2 = 0xA8,
    K_F3 = 0xA9,
    K_F4 = 0xAA,
    K_F5 = 0xAB,
    K_F6 = 0xAC,
    K_F7 = 0xAD,
    K_F8 = 0xAE,
    K_F9 = 0xAF,
    K_F10 = 0xB0,
    K_F11 = 0xB1,
    K_F12 = 0xB2,
    K_F13 = 0xB3,
    K_F14 = 0xB4,
    K_F15 = 0xB5,
    K_KP_HOME = 0xB6,
    K_KP_UPARROW = 0xB7,
    K_KP_PGUP = 0xB8,
    K_KP_LEFTARROW = 0xB9,
    K_KP_5 = 0xBA,
    K_KP_RIGHTARROW = 0xBB,
    K_KP_END = 0xBC,
    K_KP_DOWNARROW = 0xBD,
    K_KP_PGDN = 0xBE,
    K_KP_ENTER = 0xBF,
    K_KP_INS = 0xC0,
    K_KP_DEL = 0xC1,
    K_KP_SLASH = 0xC2,
    K_KP_MINUS = 0xC3,
    K_KP_PLUS = 0xC4,
    K_KP_NUMLOCK = 0xC5,
    K_KP_STAR = 0xC6,
    K_KP_EQUALS = 0xC7,
    K_MOUSE1 = 0xC8,
    K_MOUSE2 = 0xC9,
    K_MOUSE3 = 0xCA,
    K_MOUSE4 = 0xCB,
    K_MOUSE5 = 0xCC,
    K_MWHEELDOWN = 0xCD,
    K_MWHEELUP = 0xCE,
    K_AUX1 = 0xCF,
    K_AUX2 = 0xD0,
    K_AUX3 = 0xD1,
    K_AUX4 = 0xD2,
    K_AUX5 = 0xD3,
    K_AUX6 = 0xD4,
    K_AUX7 = 0xD5,
    K_AUX8 = 0xD6,
    K_AUX9 = 0xD7,
    K_AUX10 = 0xD8,
    K_AUX11 = 0xD9,
    K_AUX12 = 0xDA,
    K_AUX13 = 0xDB,
    K_AUX14 = 0xDC,
    K_AUX15 = 0xDD,
    K_AUX16 = 0xDE,
    K_LAST_KEY = 0xDF,
  };

  // 93
  //
  enum MaterialTechniqueType
  {
    TECHNIQUE_DEPTH_PREPASS = 0x0,
    TECHNIQUE_BUILD_FLOAT_Z = 0x1,
    TECHNIQUE_BUILD_SHADOWMAP_DEPTH = 0x2,
    TECHNIQUE_BUILD_SHADOWMAP_COLOR = 0x3,
    TECHNIQUE_UNLIT = 0x4,
    TECHNIQUE_EMISSIVE = 0x5,
    TECHNIQUE_EMISSIVE_DFOG = 0x6,
    TECHNIQUE_EMISSIVE_SHADOW = 0x7,
    TECHNIQUE_EMISSIVE_SHADOW_DFOG = 0x8,
    TECHNIQUE_LIT_BEGIN = 0x9,
    TECHNIQUE_LIT = 0x9,
    TECHNIQUE_LIT_DFOG = 0xA,
    TECHNIQUE_LIT_SUN = 0xB,
    TECHNIQUE_LIT_SUN_DFOG = 0xC,
    TECHNIQUE_LIT_SUN_SHADOW = 0xD,
    TECHNIQUE_LIT_SUN_SHADOW_DFOG = 0xE,
    TECHNIQUE_LIT_SPOT = 0xF,
    TECHNIQUE_LIT_SPOT_DFOG = 0x10,
    TECHNIQUE_LIT_SPOT_SHADOW = 0x11,
    TECHNIQUE_LIT_SPOT_SHADOW_DFOG = 0x12,
    TECHNIQUE_LIT_OMNI = 0x13,
    TECHNIQUE_LIT_OMNI_DFOG = 0x14,
    TECHNIQUE_LIT_OMNI_SHADOW = 0x15,
    TECHNIQUE_LIT_OMNI_SHADOW_DFOG = 0x16,
    TECHNIQUE_LIT_INSTANCED = 0x17,
    TECHNIQUE_LIT_INSTANCED_DFOG = 0x18,
    TECHNIQUE_LIT_INSTANCED_SUN = 0x19,
    TECHNIQUE_LIT_INSTANCED_SUN_DFOG = 0x1A,
    TECHNIQUE_LIT_INSTANCED_SUN_SHADOW = 0x1B,
    TECHNIQUE_LIT_INSTANCED_SUN_SHADOW_DFOG = 0x1C,
    TECHNIQUE_LIT_INSTANCED_SPOT = 0x1D,
    TECHNIQUE_LIT_INSTANCED_SPOT_DFOG = 0x1E,
    TECHNIQUE_LIT_INSTANCED_SPOT_SHADOW = 0x1F,
    TECHNIQUE_LIT_INSTANCED_SPOT_SHADOW_DFOG = 0x20,
    TECHNIQUE_LIT_INSTANCED_OMNI = 0x21,
    TECHNIQUE_LIT_INSTANCED_OMNI_DFOG = 0x22,
    TECHNIQUE_LIT_INSTANCED_OMNI_SHADOW = 0x23,
    TECHNIQUE_LIT_INSTANCED_OMNI_SHADOW_DFOG = 0x24,
    TECHNIQUE_LIT_END = 0x25,
    TECHNIQUE_LIGHT_SPOT = 0x25,
    TECHNIQUE_LIGHT_OMNI = 0x26,
    TECHNIQUE_LIGHT_SPOT_SHADOW = 0x27,
    TECHNIQUE_FAKELIGHT_NORMAL = 0x28,
    TECHNIQUE_FAKELIGHT_VIEW = 0x29,
    TECHNIQUE_SUNLIGHT_PREVIEW = 0x2A,
    TECHNIQUE_CASE_TEXTURE = 0x2B,
    TECHNIQUE_WIREFRAME_SOLID = 0x2C,
    TECHNIQUE_WIREFRAME_SHADED = 0x2D,
    TECHNIQUE_DEBUG_BUMPMAP = 0x2E,
    TECHNIQUE_DEBUG_BUMPMAP_INSTANCED = 0x2F,
    TECHNIQUE_COUNT = 0x30,
    TECHNIQUE_TOTAL_COUNT = 0x31,
    TECHNIQUE_NONE = 0x32,
  };

  // 97
  //
  enum MigrationVerboseState
  {
    MVSTATE_INACTIVE = 0x0,
    MVSTATE_WAITING = 0x1,
    MVSTATE_RATING = 0x2,
    MVSTATE_SENDING = 0x3,
    MVSTATE_MIGRATING = 0x4,
    MVSTATE_COUNT = 0x5,
  };

  // 98
  //
  enum StanceState
  {
    CL_STANCE_STAND = 0x0,
    CL_STANCE_CROUCH = 0x1,
    CL_STANCE_PRONE = 0x2,
  };

  // 99
  //
  enum ImpactType
  {
    IMPACT_TYPE_NONE = 0x0,
    IMPACT_TYPE_BULLET_SMALL = 0x1,
    IMPACT_TYPE_BULLET_LARGE = 0x2,
    IMPACT_TYPE_BULLET_AP = 0x3,
    IMPACT_TYPE_BULLET_EXPLODE = 0x4,
    IMPACT_TYPE_SHOTGUN = 0x5,
    IMPACT_TYPE_SHOTGUN_EXPLODE = 0x6,
    IMPACT_TYPE_GRENADE_BOUNCE = 0x7,
    IMPACT_TYPE_GRENADE_EXPLODE = 0x8,
    IMPACT_TYPE_ROCKET_EXPLODE = 0x9,
    IMPACT_TYPE_PROJECTILE_DUD = 0xA,
    IMPACT_TYPE_COUNT = 0xB,
  };

  // 100
  //
  enum netadrtype_t
  {
    NA_BOT = 0x0,
    NA_BAD = 0x1,
    NA_LOOPBACK = 0x2,
    NA_BROADCAST = 0x3,
    NA_IP = 0x4,
    NA_IPX = 0x5,
    NA_BROADCAST_IPX = 0x6,
  };

  // 101
  //
  enum objectiveState_t
  {
    OBJST_EMPTY = 0x0,
    OBJST_ACTIVE = 0x1,
    OBJST_INVISIBLE = 0x2,
    OBJST_DONE = 0x3,
    OBJST_CURRENT = 0x4,
    OBJST_FAILED = 0x5,
    OBJST_NUMSTATES = 0x6,
  };

  // 102
  //
  enum weapType_t
  {
    WEAPTYPE_BULLET = 0x0,
    WEAPTYPE_GRENADE = 0x1,
    WEAPTYPE_PROJECTILE = 0x2,
    WEAPTYPE_RIOTSHIELD = 0x3,
    WEAPTYPE_NUM = 0x4,
  };

  // 103
  //
  enum WeapOverlayInteface_t
  {
    WEAPOVERLAYINTERFACE_NONE = 0x0,
    WEAPOVERLAYINTERFACE_JAVELIN = 0x1,
    WEAPOVERLAYINTERFACE_TURRETSCOPE = 0x2,
    WEAPOVERLAYINTERFACECOUNT = 0x3,
  };

  // 104
  //
  enum he_type_t
  {
    HE_TYPE_FREE = 0x0,
    HE_TYPE_TEXT = 0x1,
    HE_TYPE_VALUE = 0x2,
    HE_TYPE_PLAYERNAME = 0x3,
    HE_TYPE_MAPNAME = 0x4,
    HE_TYPE_GAMETYPE = 0x5,
    HE_TYPE_MATERIAL = 0x6,
    HE_TYPE_TIMER_DOWN = 0x7,
    HE_TYPE_TIMER_UP = 0x8,
    HE_TYPE_TIMER_STATIC = 0x9,
    HE_TYPE_TENTHS_TIMER_DOWN = 0xA,
    HE_TYPE_TENTHS_TIMER_UP = 0xB,
    HE_TYPE_TENTHS_TIMER_STATIC = 0xC,
    HE_TYPE_CLOCK_DOWN = 0xD,
    HE_TYPE_CLOCK_UP = 0xE,
    HE_TYPE_WAYPOINT = 0xF,
    HE_TYPE_COUNT = 0x10,
  };

  // 105
  //
  enum weapClass_t
  {
    WEAPCLASS_RIFLE = 0x0,
    WEAPCLASS_SNIPER = 0x1,
    WEAPCLASS_MG = 0x2,
    WEAPCLASS_SMG = 0x3,
    WEAPCLASS_SPREAD = 0x4,
    WEAPCLASS_PISTOL = 0x5,
    WEAPCLASS_GRENADE = 0x6,
    WEAPCLASS_ROCKETLAUNCHER = 0x7,
    WEAPCLASS_TURRET = 0x8,
    WEAPCLASS_THROWINGKNIFE = 0x9,
    WEAPCLASS_NON_PLAYER = 0xA,
    WEAPCLASS_ITEM = 0xB,
    WEAPCLASS_NUM = 0xC,
  };

  // 106
  //
  enum weapOverlayReticle_t
  {
    WEAPOVERLAYRETICLE_NONE = 0x0,
    WEAPOVERLAYRETICLE_CROSSHAIR = 0x1,
    WEAPOVERLAYRETICLE_NUM = 0x2,
  };

  // 108
  //
  enum weapStance_t
  {
    WEAPSTANCE_STAND = 0x0,
    WEAPSTANCE_DUCK = 0x1,
    WEAPSTANCE_PRONE = 0x2,
    WEAPSTANCE_NUM = 0x3,
  };

  // 109
  //
  enum weaponIconRatioType_t
  {
    WEAPON_ICON_RATIO_1TO1 = 0x0,
    WEAPON_ICON_RATIO_2TO1 = 0x1,
    WEAPON_ICON_RATIO_4TO1 = 0x2,
    WEAPON_ICON_RATIO_COUNT = 0x3,
  };

  // 111
  //
  enum PenetrateType
  {
    PENETRATE_TYPE_NONE = 0x0,
    PENETRATE_TYPE_SMALL = 0x1,
    PENETRATE_TYPE_MEDIUM = 0x2,
    PENETRATE_TYPE_LARGE = 0x3,
    PENETRATE_TYPE_COUNT = 0x4,
  };

  // 112
  //
  enum weapInventoryType_t
  {
    WEAPINVENTORY_PRIMARY = 0x0,
    WEAPINVENTORY_OFFHAND = 0x1,
    WEAPINVENTORY_ITEM = 0x2,
    WEAPINVENTORY_ALTMODE = 0x3,
    WEAPINVENTORY_EXCLUSIVE = 0x4,
    WEAPINVENTORY_SCAVENGER = 0x5,
    WEAPINVENTORYCOUNT = 0x6,
  };

  // 113
  //
  enum activeReticleType_t
  {
    VEH_ACTIVE_RETICLE_NONE = 0x0,
    VEH_ACTIVE_RETICLE_PIP_ON_A_STICK = 0x1,
    VEH_ACTIVE_RETICLE_BOUNCING_DIAMOND = 0x2,
    VEH_ACTIVE_RETICLE_COUNT = 0x3,
  };

  // 114
  //
  enum ammoCounterClipType_t
  {
    AMMO_COUNTER_CLIP_NONE = 0x0,
    AMMO_COUNTER_CLIP_MAGAZINE = 0x1,
    AMMO_COUNTER_CLIP_SHORTMAGAZINE = 0x2,
    AMMO_COUNTER_CLIP_SHOTGUN = 0x3,
    AMMO_COUNTER_CLIP_ROCKET = 0x4,
    AMMO_COUNTER_CLIP_BELTFED = 0x5,
    AMMO_COUNTER_CLIP_ALTWEAPON = 0x6,
    AMMO_COUNTER_CLIP_COUNT = 0x7,
  };

  // 115
  //
  enum weapProjExposion_t
  {
    WEAPPROJEXP_GRENADE = 0x0,
    WEAPPROJEXP_ROCKET = 0x1,
    WEAPPROJEXP_FLASHBANG = 0x2,
    WEAPPROJEXP_NONE = 0x3,
    WEAPPROJEXP_DUD = 0x4,
    WEAPPROJEXP_SMOKE = 0x5,
    WEAPPROJEXP_HEAVY = 0x6,
    WEAPPROJEXP_NUM = 0x7,
  };

  // 116
  //
  enum guidedMissileType_t
  {
    MISSILE_GUIDANCE_NONE = 0x0,
    MISSILE_GUIDANCE_SIDEWINDER = 0x1,
    MISSILE_GUIDANCE_HELLFIRE = 0x2,
    MISSILE_GUIDANCE_JAVELIN = 0x3,
    MISSILE_GUIDANCE_COUNT = 0x4,
  };

  // 165
  //
  enum expDataType
  {
    VAL_INT = 0x0,
    VAL_FLOAT = 0x1,
    VAL_STRING = 0x2,
    NUM_INTERNAL_DATATYPES = 0x3,
    VAL_FUNCTION = 0x3,
    NUM_DATATYPES = 0x4,
  };

  // 183
  //
  enum UILocalVarType
  {
    UILOCALVAR_INT = 0x0,
    UILOCALVAR_FLOAT = 0x1,
    UILOCALVAR_STRING = 0x2,
  };

  // 249
  //
  enum LbColType
  {
    LBCOL_TYPE_NUMBER = 0x0,
    LBCOL_TYPE_TIME = 0x1,
    LBCOL_TYPE_LEVELXP = 0x2,
    LBCOL_TYPE_PRESTIGE = 0x3,
    LBCOL_TYPE_BIGNUMBER = 0x4,
    LBCOL_TYPE_PERCENT = 0x5,
    LBCOL_TYPE_COUNT = 0x6,
  };

  // 250
  //
  enum LbAggType
  {
    LBAGG_TYPE_MIN = 0x0,
    LBAGG_TYPE_MAX = 0x1,
    LBAGG_TYPE_SUM = 0x2,
    LBAGG_TYPE_LAST = 0x3,
    LBAGG_TYPE_COUNT = 0x4,
  };

  // 261
  //
  enum VehicleAxleType
  {
    VEH_AXLE_FRONT = 0x0,
    VEH_AXLE_REAR = 0x1,
    VEH_AXLE_ALL = 0x2,
    VEH_AXLE_COUNT = 0x3,
  };

  // 262
  //
  enum VehicleType
  {
    VEH_WHEELS_4 = 0x0,
    VEH_TANK = 0x1,
    VEH_PLANE = 0x2,
    VEH_BOAT = 0x3,
    VEH_ARTILLERY = 0x4,
    VEH_HELICOPTER = 0x5,
    VEH_SNOWMOBILE = 0x6,
    VEH_TYPE_COUNT = 0x7,
  };

  // 313
  //
  enum DynEntityType
  {
    DYNENT_TYPE_INVALID = 0x0,
    DYNENT_TYPE_CLUTTER = 0x1,
    DYNENT_TYPE_DESTRUCT = 0x2,
    DYNENT_TYPE_COUNT = 0x3,
  };

  // 366
  //
  enum dvarType : std::uint8_t
  {
    DVAR_TYPE_BOOL = 0x0,
    DVAR_TYPE_FLOAT = 0x1,
    DVAR_TYPE_FLOAT_2 = 0x2,
    DVAR_TYPE_FLOAT_3 = 0x3,
    DVAR_TYPE_FLOAT_4 = 0x4,
    DVAR_TYPE_INT = 0x5,
    DVAR_TYPE_ENUM = 0x6,
    DVAR_TYPE_STRING = 0x7,
    DVAR_TYPE_COLOR = 0x8,
    DVAR_TYPE_FLOAT_3_COLOR = 0x9,
    DVAR_TYPE_INT64 = 0xA,
    DVAR_TYPE_UINT64 = 0xB,
    DVAR_TYPE_COUNT = 0xC,
  };

  // 367
  //
  enum DvarSetSource
  {
    DVAR_SOURCE_INTERNAL = 0x0,
    DVAR_SOURCE_EXTERNAL = 0x1,
    DVAR_SOURCE_SCRIPT = 0x2,
    DVAR_SOURCE_DEVGUI = 0x3,
  };

  // 368
  //
  enum StructuredDataTypeCategory
  {
    DATA_INT = 0x0,
    DATA_BYTE = 0x1,
    DATA_BOOL = 0x2,
    DATA_STRING = 0x3,
    DATA_ENUM = 0x4,
    DATA_STRUCT = 0x5,
    DATA_INDEXED_ARRAY = 0x6,
    DATA_ENUM_ARRAY = 0x7,
    DATA_FLOAT = 0x8,
    DATA_SHORT = 0x9,
    DATA_COUNT = 0xA,
  };

  // 548
  //
  enum nodeType
  {
    NODE_ERROR = 0x0,
    NODE_PATHNODE = 0x1,
    NODE_COVER_STAND = 0x2,
    NODE_COVER_CROUCH = 0x3,
    NODE_COVER_CROUCH_WINDOW = 0x4,
    NODE_COVER_PRONE = 0x5,
    NODE_COVER_RIGHT = 0x6,
    NODE_COVER_LEFT = 0x7,
    NODE_AMBUSH = 0x8,
    NODE_EXPOSED = 0x9,
    NODE_CONCEALMENT_STAND = 0xA,
    NODE_CONCEALMENT_CROUCH = 0xB,
    NODE_CONCEALMENT_PRONE = 0xC,
    NODE_DOOR = 0xD,
    NODE_DOOR_INTERIOR = 0xE,
    NODE_SCRIPTED = 0xF,
    NODE_NEGOTIATION_BEGIN = 0x10,
    NODE_NEGOTIATION_END = 0x11,
    NODE_TURRET = 0x12,
    NODE_GUARD = 0x13,
    NODE_NUMTYPES = 0x14,
    NODE_DONTLINK = 0x14,
  };

  // 551
  //
  enum PathNodeErrorCode
  {
    PNERR_NONE = 0x0,
    PNERR_INSOLID = 0x1,
    PNERR_FLOATING = 0x2,
    PNERR_NOLINK = 0x3,
    PNERR_DUPLICATE = 0x4,
    PNERR_NOSTANCE = 0x5,
    PNERR_INVALIDDOOR = 0x6,
    PNERR_NOANGLES = 0x7,
    PNERR_BADPLACEMENT = 0x8,
    NUM_PATH_NODE_ERRORS = 0x9,
  };

  // 557
  //
  enum LocSelInputState
  {
    LOC_SEL_INPUT_NONE = 0x0,
    LOC_SEL_INPUT_CONFIRM = 0x1,
    LOC_SEL_INPUT_CANCEL = 0x2,
  };

  // 756
  //
  struct trajectory_t
  {
    trType_t trType;
    int trTime;
    int trDuration;
    float trBase[3];
    float trDelta[3];
  };

  // 771
  //
  struct _AILSOUNDINFO
  {
    int format;
    const void* data_ptr;
    unsigned int data_len;
    unsigned int rate;
    int bits;
    int channels;
    unsigned int samples;
    unsigned int block_size;
    const void* initial_ptr;
  };

  // 772
  //
  struct MssSound
  {
    _AILSOUNDINFO info;
    char* data;
  };

  // 773
  //
  struct LoadedSound
  {
    const char* name;
    MssSound sound;
  };

  // 774
  //
  struct StreamFileNameRaw
  {
    const char* dir;
    const char* name;
  };

  // 775
  //
  union StreamFileInfo
  {
    StreamFileNameRaw raw;
  };

  // 776
  //
  struct StreamFileName
  {
    StreamFileInfo info;
  };

  // 777
  //
  struct StreamedSound
  {
    StreamFileName filename;
  };

  // 778
  //
  union SoundFileRef
  {
    LoadedSound* loadSnd;
    StreamedSound streamSnd;
  };

  // 779
  //
  struct SoundFile
  {
    char type;
    char exists;
    SoundFileRef u;
  };

  // 780
  //
  struct SndCurve
  {
    const char* filename;
    unsigned short knotCount;
    float knots[16][2];
  };

  // 781
  //
  struct MSSSpeakerLevels
  {
    int speaker;
    int numLevels;
    float levels[2];
  };

  // 782
  //
  struct MSSChannelMap
  {
    int speakerCount;
    MSSSpeakerLevels speakers[6];
  };

  // 783
  //
  struct SpeakerMap
  {
    bool isDefault;
    const char* name;
    MSSChannelMap channelMaps[2][2];
  };

  // 785
  //
  struct snd_alias_t
  {
    const char* aliasName;
    const char* subtitle;
    const char* secondaryAliasName;
    const char* chainAliasName;
    const char* mixerGroup;
    SoundFile* soundFile;
    int sequence;
    float volMin;
    float volMax;
    float pitchMin;
    float pitchMax;
    float distMin;
    float distMax;
    float velocityMin;
    int flags;

    union
    {
      float slavePercentage;
      float masterPercentage;
    };

    float probability;
    float lfePercentage;
    float centerPercentage;
    int startDelay;
    SndCurve* volumeFalloffCurve;
    float envelopMin;
    float envelopMax;
    float envelopPercentage;
    SpeakerMap* speakerMap;
  };

  // 791
  //
  union DvarValue
  {
    bool enabled;
    int integer;
    unsigned int unsignedInt;
    std::int64_t integer64;
    std::uint64_t unsignedInt64;
    float value;
    vec4_t vector;
    const char* string;
    char color [4];
  };

  // 795
  //
  union DvarLimits
  {
    struct
    {
      int stringCount;
      const char** strings;
    } enumeration;

    struct
    {
      int min;
      int max;
    } integer;

    struct
    {
      std::int64_t min;
      std::int64_t max;
    } integer64;

    struct
    {
      std::uint64_t min;
      std::uint64_t max;
    } unsignedInt64;

    struct
    {
      float min;
      float max;
    } value;

    struct
    {
      float min;
      float max;
    } vector;
  };

  // 796
  //
  struct dvar_t
  {
    const char *name;
    DvarFlags flags;
    dvarType type;
    bool modified;
    std::uint8_t pad0[2];
    DvarValue current;
    DvarValue latched;
    DvarValue reset;
    DvarLimits domain;
    bool (*domainFunc) (dvar_t *, DvarValue *);
    dvar_t *hashNext;
  };

  static_assert (sizeof (dvarType) == 1,
                 "dvarType must match the engine's 1-byte ABI");
  static_assert (offsetof (dvar_t, current) == 0x10,
                 "dvar_t::current ABI offset mismatch");
  static_assert (offsetof (dvar_t, latched) == 0x20,
                 "dvar_t::latched ABI offset mismatch");
  static_assert (offsetof (dvar_t, reset) == 0x30,
                 "dvar_t::reset ABI offset mismatch");
  static_assert (offsetof (dvar_t, domain) == 0x40,
                 "dvar_t::domain ABI offset mismatch");
  static_assert (offsetof (dvar_t, domainFunc) == 0x50,
                 "dvar_t::domainFunc ABI offset mismatch");
  static_assert (offsetof (dvar_t, hashNext) == 0x58,
                 "dvar_t::hashNext ABI offset mismatch");
  static_assert (sizeof (dvar_t) == 0x60,
                 "dvar_t ABI size mismatch");

  struct snd_alias_list_t
  {
    const char* aliasName;
    snd_alias_t* head;
    int count;
  };

  // 961
  //
  struct CmdArgs
  {
    int nesting;
    int localClientNum[8];
    int controllerIndex[8];
    int argc[8];
    const char **argv[8];
  };

  // 962
  //
  struct GfxDrawSurfFields
  {
    unsigned long long objectId : 16;
    unsigned long long reflectionProbeIndex : 8;
    unsigned long long hasGfxEntIndex : 1;
    unsigned long long customIndex : 5;
    unsigned long long materialSortedIndex : 12;
    unsigned long long prepass : 2;
    unsigned long long useHeroLighting : 1;
    unsigned long long sceneLightIndex : 8;
    unsigned long long surfType : 4;
    unsigned long long primarySortKey : 6;
    unsigned long long unused : 1;
  };

  // 963
  //
  union GfxDrawSurf
  {
    GfxDrawSurfFields fields;
    unsigned long long packed;
  };

  // 964
  //
  struct MaterialInfo
  {
    const char* name;
    char gameFlags;
    char sortKey;
    char textureAtlasRowCount;
    char textureAtlasColumnCount;
    GfxDrawSurf drawSurf;
    unsigned int surfaceTypeBits;
    unsigned short hashIndex;
  };

  // 965
  //
  struct MaterialTechniqueSet
  {
    const char* name;
    char worldVertFormat;
    bool hasBeenUploaded;
    char unused[1];
    MaterialTechniqueSet* remappedTechniqueSet;
    MaterialTechnique* techniques[48];
  };

  // 966
  //
  struct MaterialStreamRouting
  {
    char source;
    char dest;
  };

  // 968
  //
  struct MaterialVertexStreamRouting
  {
    MaterialStreamRouting data[13];
    IDirect3DVertexDeclaration9* decl[16];
  };

  // 969
  //
  struct MaterialVertexDeclaration
  {
    const char* name;
    char streamCount;
    bool hasOptionalSource;
    MaterialVertexStreamRouting routing;
  };

  // 971
  //
  struct GfxVertexShaderLoadDef
  {
    unsigned int* program;
    unsigned short programSize;
    unsigned short loadForRenderer;
  };

  // 972
  //
  struct MaterialVertexShaderProgram
  {
    IDirect3DVertexShader9* vs;
    GfxVertexShaderLoadDef loadDef;
  };

  // 973
  //
  struct MaterialVertexShader
  {
    const char* name;
    MaterialVertexShaderProgram prog;
  };

  // 975
  //
  struct GfxPixelShaderLoadDef
  {
    unsigned int* program;
    unsigned short programSize;
    unsigned short loadForRenderer;
  };

  // 976
  //
  struct MaterialPixelShaderProgram
  {
    IDirect3DPixelShader9* ps;
    GfxPixelShaderLoadDef loadDef;
  };

  // 977
  //
  struct MaterialPixelShader
  {
    const char* name;
    MaterialPixelShaderProgram prog;
  };

  // 978
  //
  struct MaterialArgumentCodeConst
  {
    unsigned short index;
    char firstRow;
    char rowCount;
  };

  // 979
  //
  union MaterialArgumentDef
  {
    const float* literalConst;
    MaterialArgumentCodeConst codeConst;
    unsigned int codeSampler;
    unsigned int nameHash;
  };

  // 980
  //
  struct MaterialShaderArgument
  {
    unsigned short type;
    unsigned short dest;
    MaterialArgumentDef u;
  };

  // 981
  //
  struct MaterialPass
  {
    MaterialVertexDeclaration* vertexDecl;
    MaterialVertexShader* vertexShader;
    MaterialPixelShader* pixelShader;
    char perPrimArgCount;
    char perObjArgCount;
    char stableArgCount;
    char customSamplerFlags;
    MaterialShaderArgument* args;
  };

  // 982
  //
  struct MaterialTechnique
  {
    const char* name;
    unsigned short flags;
    unsigned short passCount;
    MaterialPass passArray[1];
  };

  // 988
  //
  struct GfxImageLoadDef
  {
    char levelCount;
    char pad[3];
    int flags;
    int format;
    int resourceSize;
    char data[1];
  };

  // 989
  //
  union GfxTexture
  {
    IDirect3DBaseTexture9* basemap;
    IDirect3DTexture9* map;
    IDirect3DVolumeTexture9* volmap;
    IDirect3DCubeTexture9* cubemap;
    GfxImageLoadDef* loadDef;
  };

  // 990
  //
  struct Picmip
  {
    char platform[2];
  };

  // 991
  //
  struct CardMemory
  {
    int platform[2];
  };

  // 992
  //
  struct GfxImage
  {
    GfxTexture texture;
    char mapType;
    char semantic;
    char category;
    bool useSrgbReads;
    Picmip picmip;
    bool noPicmip;
    char track;
    CardMemory cardMemory;
    unsigned short width;
    unsigned short height;
    unsigned short depth;
    bool delayLoadPixels;
    const char* name;
  };

  // 993
  //
  struct WaterWritable
  {
    float floatTime;
  };

  // 994
  //
  struct complex_s
  {
    float real;
    float imag;
  };

  // 995
  //
  struct water_t
  {
    WaterWritable writable;
    complex_s* H0;
    float* wTerm;
    int M;
    int N;
    float Lx;
    float Lz;
    float gravity;
    float windvel;
    float winddir[2];
    float amplitude;
    float codeConstant[4];
    GfxImage* image;
  };

  // 996
  //
  union MaterialTextureDefInfo
  {
    GfxImage* image;
    water_t* water;
  };

  // 997
  //
  struct MaterialTextureDef
  {
    unsigned int nameHash;
    char nameStart;
    char nameEnd;
    char samplerState;
    char semantic;
    MaterialTextureDefInfo u;
  };

  // 998
  //
  struct MaterialConstantDef
  {
    unsigned int nameHash;
    char name[12];
    float literal[4];
  };

  // 999
  //
  struct GfxStateBits
  {
    unsigned int loadBits[2];
  };

  // 1000
  //
  struct Material
  {
    MaterialInfo info;
    char stateBitsEntry[48];
    char textureCount;
    char constantCount;
    char stateBitsCount;
    char stateFlags;
    char cameraRegion;
    MaterialTechniqueSet* techniqueSet;
    MaterialTextureDef* textureTable;
    MaterialConstantDef* constantTable;
    GfxStateBits* stateBitsTable;
  };

  // 1001
  //
  struct clientLogo_t
  {
    int startTime;
    int duration;
    int fadein;
    int fadeout;
    Material *material[2];
  };

  // 1002
  //
  struct Glyph
  {
    unsigned short letter;
    char x0;
    char y0;
    char dx;
    char pixelWidth;
    char pixelHeight;
    float s0;
    float t0;
    float s1;
    float t1;
  };

  // 1003
  //
  struct Font_s
  {
    const char* fontName;
    int pixelHeight;
    int glyphCount;
    Material* material;
    Material* glowMaterial;
    Glyph* glyphs;
  };

  // 1004
  //
  struct vidConfig_t
  {
    unsigned int sceneWidth;
    unsigned int sceneHeight;
    unsigned int displayWidth;
    unsigned int displayHeight;
    unsigned int displayFrequency;
    int isFullscreen;
    float aspectRatioWindow;
    float aspectRatioScenePixel;
    float aspectRatioDisplayPixel;
    unsigned int maxTextureSize;
    unsigned int maxTextureMaps;
    bool deviceSupportsGamma;
  };

  // 1005
  //
  struct trDebugString_t
  {
    float xyz[3];
    float color[4];
    float scale;
    char text[96];
  };

  // 1006
  //
  struct clientDebugStringInfo_t
  {
    int max;
    int num;
    trDebugString_t *strings;
    int *durations;
  };

  // 1007
  struct trDebugLine_t
  {
    float start[3];
    float end[3];
    float color[4];
    int depthTest;
  };

  // 1008
  //
  struct clientDebugLineInfo_t
  {
    int max;
    int num;
    trDebugLine_t *lines;
    int *durations;
  };

  // 1009
  //
  struct clientDebug_t
  {
    int prevFromServer;
    int fromServer;
    clientDebugStringInfo_t clStrings;
    clientDebugStringInfo_t svStringsBuffer;
    clientDebugStringInfo_t svStrings;
    clientDebugLineInfo_t clLines;
    clientDebugLineInfo_t svLinesBuffer;
    clientDebugLineInfo_t svLines;
  };

  struct ClientMatchData
  {
    char def[64];
    char data[1024];
  };

  // 1015
  struct XNADDR
  {
    in_addr ina;
    in_addr inaOnline;
    unsigned short wPort;
    unsigned short wPortOnline;
    char abEnet[4];
    char abOnline[20];
  };

  // 1016
  //
  struct gameState_t
  {
    int stringOffsets[4339];
    char stringData[131072];
    int dataCount;
  };

  // 1017
  //
  struct SprintState
  {
    int sprintButtonUpRequired;
    int sprintDelay;
    int lastSprintStart;
    int lastSprintEnd;
    int sprintStartMaxLength;
  };


  // 1018
  //
  struct MantleState
  {
    float yaw;
    int timer;
    int transIndex;
    int flags;
  };

  // 1019
  //
  struct PlayerActiveWeaponState
  {
    int weapAnim;
    int weaponTime;
    int weaponDelay;
    int weaponRestrictKickTime;
    int weaponState;
    int weapHandFlags;
    unsigned int weaponShotCount;
  };

  // 1020
  //
  struct PlayerEquippedWeaponState
  {
    bool usedBefore;
    bool dualWielding;
    char weaponModel;
    bool needsRechamber[2];
  };

  // 1021
  //
  struct GlobalAmmo
  {
    int ammoType;
    int ammoCount;
  };

  // 1022
  //
  struct ClipAmmo
  {
    int clipIndex;
    int ammoCount[2];
  };

  // 1023
  //
  struct PlayerWeaponCommonState
  {
    int offHandIndex;
    OffhandClass offhandPrimary;
    OffhandClass offhandSecondary;
    unsigned int weapon;
    unsigned int primaryWeaponForAltMode;
    int weapFlags;
    float fWeaponPosFrac;
    float aimSpreadScale;
    int adsDelayTime;
    int spreadOverride;
    int spreadOverrideState;
    PlayerHandIndex lastWeaponHand;
    GlobalAmmo ammoNotInClip[15];
    ClipAmmo ammoInClip[15];
    int weapLockFlags;
    int weapLockedEntnum;
    float weapLockedPos[3];
    int weaponIdleTime;
  };

  // 1024
  //
  struct ActionSlotParam_SpecifyWeapon
  {
    unsigned int index;
  };

  // 1025
  //
  struct ActionSlotParam
  {
    ActionSlotParam_SpecifyWeapon specifyWeapon;
  };

  // 1026
  //
  struct objective_t
  {
    objectiveState_t state;
    float origin[3];
    int entNum;
    int teamNum;
    int icon;
  };

  // 1028
  //
  union hudelem_color_t
  {
    // 1027
    //
    struct
    {
      char r;
      char g;
      char b;
      char a;
    } __s0;

    int rgba;
  };

  // 1029
  //
  struct hudelem_s
  {
    he_type_t type;
    float x;
    float y;
    float z;
    int targetEntNum;
    float fontScale;
    float fromFontScale;
    int fontScaleStartTime;
    int fontScaleTime;
    int font;
    int alignOrg;
    int alignScreen;
    hudelem_color_t color;
    hudelem_color_t fromColor;
    int fadeStartTime;
    int fadeTime;
    int label;
    int width;
    int height;
    int materialIndex;
    int fromWidth;
    int fromHeight;
    int scaleStartTime;
    int scaleTime;
    float fromX;
    float fromY;
    int fromAlignOrg;
    int fromAlignScreen;
    int moveStartTime;
    int moveTime;
    int time;
    int duration;
    float value;
    int text;
    float sort;
    hudelem_color_t glowColor;
    int fxBirthTime;
    int fxLetterTime;
    int fxDecayStartTime;
    int fxDecayDuration;
    int soundID;
    int flags;
  };

  // 1031
  //
  struct playerState_s
  {
    int commandTime;
    int pm_type;
    int pm_time;
    int pm_flags;
    int otherFlags;
    int linkFlags;
    int bobCycle;
    float origin[3];
    float velocity[3];
    int grenadeTimeLeft;
    int throwbackGrenadeOwner;
    int throwbackGrenadeTimeLeft;
    unsigned int throwbackWeaponIndex;
    int remoteEyesEnt;
    int remoteEyesTagname;
    int remoteControlEnt;
    int foliageSoundTime;
    int gravity;
    float leanf;
    int speed;
    float delta_angles[3];
    int groundEntityNum;
    float vLadderVec[3];
    int jumpTime;
    float jumpOriginZ;
    int legsTimer;
    int legsAnim;
    int torsoTimer;
    int torsoAnim;
    int legsAnimDuration;
    int torsoAnimDuration;
    int damageTimer;
    int damageDuration;
    int flinchYawAnim;
    int corpseIndex;
    int movementDir;
    int eFlags;
    int eventSequence;
    int events[4];
    unsigned int eventParms[4];
    int oldEventSequence;
    int unpredictableEventSequence;
    int unpredictableEventSequenceOld;
    int unpredictableEvents[4];
    unsigned int unpredictableEventParms[4];
    int clientNum;
    int viewmodelIndex;
    float viewangles[3];
    int viewHeightTarget;
    float viewHeightCurrent;
    int viewHeightLerpTime;
    int viewHeightLerpTarget;
    int viewHeightLerpDown;
    float viewAngleClampBase[2];
    float viewAngleClampRange[2];
    int damageEvent;
    int damageYaw;
    int damagePitch;
    int damageCount;
    int damageFlags;
    int stats[4];
    float proneDirection;
    float proneDirectionPitch;
    float proneTorsoPitch;
    ViewLockTypes viewlocked;
    int viewlocked_entNum;
    float linkAngles[3];
    float linkWeaponAngles[3];
    int linkWeaponEnt;
    int loopSound;
    int cursorHint;
    int cursorHintString;
    int cursorHintEntIndex;
    int cursorHintDualWield;
    int iCompassPlayerInfo;
    int radarEnabled;
    int radarBlocked;
    int radarMode;
    int locationSelectionInfo;
    SprintState sprintState;
    float holdBreathScale;
    int holdBreathTimer;
    float moveSpeedScaleMultiplier;
    MantleState mantleState;
    PlayerActiveWeaponState weapState[2];
    unsigned int weaponsEquipped[15];
    PlayerEquippedWeaponState weapEquippedData[15];
    PlayerWeaponCommonState weapCommon;
    float meleeChargeYaw;
    int meleeChargeDist;
    int meleeChargeTime;
    unsigned int perks[2];
    unsigned int perkSlots[8];
    ActionSlotType actionSlotType[4];
    ActionSlotParam actionSlotParam[4];
    int weaponHudIconOverrides[6];
    int animScriptedType;
    int shellshockIndex;
    int shellshockTime;
    int shellshockDuration;
    float dofNearStart;
    float dofNearEnd;
    float dofFarStart;
    float dofFarEnd;
    float dofNearBlur;
    float dofFarBlur;
    float dofViewmodelStart;
    float dofViewmodelEnd;
    objective_t objective[32];
    int deltaTime;
    int killCamEntity;
    int killCamLookAtEntity;
    int killCamClientNum;

    // 1030
    //
    struct
    {
      hudelem_s current[31];
      hudelem_s archival[31];
    } hud;

    unsigned int partBits[6];
    int recoilScale;
    int diveDirection;
    int stunTime;
  };

  // 1032
  //
  struct clSnapshot_t
  {
    playerState_s ps;
    int valid;
    int snapFlags;
    int serverTime;
    int messageNum;
    int deltaNum;
    int ping;
    int cmdNum;
    int numEntities;
    int numClients;
    int parseEntitiesIndex;
    int parseClientsIndex;
    int serverCommandNum;
  };


  // 1033
  //
  struct LerpEntityStateTurret
  {
    float gunAngles[3];
    int lastBarrelRotChangeTime;
    int lastBarrelRotChangeRate;
    int lastHeatChangeLevel;
    int lastHeatChangeTime;
    bool isBarrelRotating;
    bool isOverheat;
    bool isHeatingUp;
    bool isBeingCarried;
  };

  // 1034
  //
  struct LerpEntityStateLoopFx
  {
    float cullDist;
    int period;
  };

  // 1035
  //
  struct LerpEntityStatePrimaryLight
  {
    char colorAndExp[4];
    float intensity;
    float radius;
    float cosHalfFovOuter;
    float cosHalfFovInner;
  };

  // 1036
  //
  struct LerpEntityStatePlayer
  {
    float leanf;
    int movementDir;
    float torsoPitch;
    float waistPitch;
    unsigned int offhandWeapon;
    int lastSpawnTime;
  };

  // 1037
  //
  struct LerpEntityStateVehicle
  {
    unsigned int indices;
    unsigned int flags;
    float bodyPitch;
    float bodyRoll;
    float steerYaw;
    float gunPitch;
    float gunYaw;
    int playerIndex;
    int pad;
  };

  // 1038
  //
  struct LerpEntityStatePlane
  {
    int ownerNum;
    int enemyIcon;
    int friendIcon;
  };

  // 1039
  //
  struct LerpEntityStateMissile
  {
    int launchTime;
    MissileFlightMode flightMode;
  };

  // 1040
  //
  struct LerpEntityStateSoundBlend
  {
    float lerp;
  };

  // 1041
  //
  struct LerpEntityStateBulletHit
  {
    float start[3];
  };

  // 1042
  //
  struct LerpEntityStateEarthquake
  {
    float scale;
    float radius;
    int duration;
  };

  // 1043
  //
  struct LerpEntityStateCustomExplode
  {
    int startTime;
  };

  // 1044
  //
  struct LerpEntityStateExplosion
  {
    float innerRadius;
    float outerRadius;
    float magnitude;
  };

  // 1045
  //
  struct LerpEntityStateExplosionJolt
  {
    float innerRadius;
    float outerRadius;
    float impulse[3];
  };

  // 1046
  //
  struct LerpEntityStatePhysicsJitter
  {
    float innerRadius;
    float outerRadius;
    float minDisplacement;
    float maxDisplacement;
  };

  // 1047
  //
  struct LerpEntityStateRadiusDamage
  {
    float range;
    int damageMin;
    int damageMax;
  };

  // 1048
  //
  struct LerpEntityStateScriptMover
  {
    int entToTakeMarksFrom;
    int xModelIndex;
    int animIndex;
    int animTime;
  };

  // 1049
  //
  struct LerpEntityStateAnonymous
  {
    int data[9];
  };

  // 1050
  //
  union LerpEntityStateTypeUnion
  {
    LerpEntityStateTurret turret;
    LerpEntityStateLoopFx loopFx;
    LerpEntityStatePrimaryLight primaryLight;
    LerpEntityStatePlayer player;
    LerpEntityStateVehicle vehicle;
    LerpEntityStatePlane plane;
    LerpEntityStateMissile missile;
    LerpEntityStateSoundBlend soundBlend;
    LerpEntityStateBulletHit bulletHit;
    LerpEntityStateEarthquake earthquake;
    LerpEntityStateCustomExplode customExplode;
    LerpEntityStateExplosion explosion;
    LerpEntityStateExplosionJolt explosionJolt;
    LerpEntityStatePhysicsJitter physicsJitter;
    LerpEntityStateRadiusDamage radiusDamage;
    LerpEntityStateScriptMover scriptMover;
    LerpEntityStateAnonymous anonymous;
  };

  // 1051
  //
  struct LerpEntityState
  {
    int eFlags;
    trajectory_t pos;
    trajectory_t apos;
    LerpEntityStateTypeUnion u;
  };

  // 1057
  //
  struct clientLinkInfo_t
  {
    __int16 parentId;
    char tagName;
    char flags;
  };

  // 1058
  //
  struct entityState_s
  {
    int number;
    int eType;
    LerpEntityState lerp;
    int time2;
    int otherEntityNum;
    int attackerEntityNum;
    int groundEntityNum;
    int loopSound;
    int surfType;

    // 1052
    //
    union
    {
      int brushModel;
      int triggerModel;
      int item;
      int xmodel;
      int primaryLight;
    } index;

    int clientNum;
    int iHeadIcon;
    int iHeadIconTeam;
    int solid;
    unsigned int eventParm;
    int eventSequence;
    int events[4];
    unsigned int eventParms[4];

    // 1053
    //
    struct
    {
      unsigned __int16 weapon;
      unsigned __int16 primaryWeapon;
    } wes;

    int legsAnim;
    int torsoAnim;

    // 1054
    //
    union
    {
      int eventParm2;
      int hintString;
      int fxId;
      int helicopterStage;
    } un1;

    // 1056
    //
    union
    {
      int hintType;

      // 1055
      //
      struct
      {
        unsigned __int16 vehicleXModel;
        char weaponModel;
      } __s1;

      int actorFlags;
    } un2;

    clientLinkInfo_t clientLinkInfo;
    unsigned int partBits[6];
    int clientMask[1];
  };

  // 1059
  //
  struct clientStatic_t
  {
    int quit;
    int hunkUsersStarted;
    char servername[256];
    int rendererStarted;
    int soundStarted;
    int uiStarted;
    int devGuiStarted;
    int frametime;
    float frametime_base;
    int realtime;
    bool gpuSyncedPrevFrame;
    bool inputUpdatedPrevFrame;
    clientLogo_t logo;
    float mapCenter[3];
    int lastServerPinged;
    int pingedServerCount;
    int totalServersParsed;
    int pingUpdateSource;
    Material* whiteMaterial;
    Material* consoleMaterial;
    Font_s* consoleFont;
    vidConfig_t vidConfig;
    clientDebug_t debug;
    int doVidRestart;
    char downloadList[1024];
    ClientMatchData matchData;
    XNADDR xnaddrs[18];
    volatile int scriptError;
    float debugRenderPos[3];
    int skelValid;
    int skelTimeStamp;
    volatile int skelMemPos;
    char skelMemory[262144];
    char* skelMemoryStart;
    bool allowedAllocSkel;
    int serverId;
    gameState_t gameState;
    clSnapshot_t noDeltaSnapshot;
    int nextNoDeltaEntity;
    entityState_s noDeltaEntities[1024];
  };

  // 1060
  //
  struct cplane_s
  {
    float normal[3];
    float dist;
    char type;
    char pad[3];
  };

  // 1061
  //
  struct cbrushside_t
  {
    cplane_s* plane;
    unsigned short materialNum;
    char firstAdjacentSideOffset;
    char edgeCount;
  };

  // 1062
  //
  struct cbrush_t
  {
    unsigned short numsides;
    unsigned short glassPieceIndex;
    cbrushside_t* sides;
    char* baseAdjacentSide;
    short axialMaterialNum[2][3];
    char firstAdjacentSideOffsets[2][3];
    char edgeCount[2][3];
  };

  // 1063
  //
  struct ProfileScriptWritable
  {
    int refCount;
    unsigned int startTime;
    unsigned int totalTime;
  };

  // 1064
  //
  struct ProfileScript
  {
    ProfileScriptWritable write[40];
    volatile unsigned int totalTime[40];
    volatile unsigned int avgTime[40];
    volatile unsigned int maxTime[40];
    volatile float cumulative[40];
    char profileScriptNames[40][20];
    int scriptSrcBufferIndex[32];
    unsigned int srcTotal;
    unsigned int srcAvgTime;
    unsigned int srcMaxTime;
  };

  // 1065
  //
  struct Bounds
  {
    float midPoint[3];
    float halfSize[3];
  };

  // 1066
  //
  struct BrushWrapper
  {
    Bounds bounds;
    cbrush_t brush;
    int totalEdgeCount;
    cplane_s* planes;
  };

  // 1067
  //
  struct XModelCollTri_s
  {
    float plane[4];
    float svec[4];
    float tvec[4];
  };

  // 1068
  //
  struct TriggerSlab
  {
    float dir[3];
    float midPoint;
    float halfSize;
  };

  // 1069
  //
  struct PhysMass
  {
    float centerOfMass[3];
    float momentsOfInertia[3];
    float productsOfInertia[3];
  };

  // 1070
  //
  struct TriggerHull
  {
    Bounds bounds;
    int contents;
    unsigned short slabCount;
    unsigned short firstSlab;
  };

  // 1071
  //
  struct FxSpawnDefLooping
  {
    int intervalMsec;
    int count;
  };

  // 1072
  //
  struct FxIntRange
  {
    int base;
    int amplitude;
  };

  // 1073
  //
  struct FxSpawnDefOneShot
  {
    FxIntRange count;
  };

  // 1074
  //
  union FxSpawnDef
  {
    FxSpawnDefLooping looping;
    FxSpawnDefOneShot oneShot;
  };

  // 1075
  //
  struct FxFloatRange
  {
    float base;
    float amplitude;
  };

  // 1076
  //
  struct FxElemAtlas
  {
    char behavior;
    char index;
    char fps;
    char loopCount;
    char colIndexBits;
    char rowIndexBits;
    short entryCount;
  };

  // 1077
  //
  struct FxElemVec3Range
  {
    float base[3];
    float amplitude[3];
  };

  // 1078
  //
  struct FxElemVelStateInFrame
  {
    FxElemVec3Range velocity;
    FxElemVec3Range totalDelta;
  };

  // 1079
  //
  struct FxElemVelStateSample
  {
    FxElemVelStateInFrame local;
    FxElemVelStateInFrame world;
  };

  // 1080
  //
  struct FxElemVisualState
  {
    char color[4];
    float rotationDelta;
    float rotationTotal;
    float size[2];
    float scale;
  };

  // 1081
  //
  struct FxElemVisStateSample
  {
    FxElemVisualState base;
    FxElemVisualState amplitude;
  };

  // 1082
  //
  struct FxElemMarkVisuals
  {
    Material* materials[2];
  };

  // 1083
  //
  struct DObjAnimMat
  {
    float quat[4];
    float trans[3];
    float transWeight;
  };

  // 1084
  //
  struct XSurfaceVertexInfo
  {
    short vertCount[4];
    unsigned short* vertsBlend;
  };

  // 1085
  //
  union GfxColor
  {
    unsigned int packed;
    char array[4];
  };

  // 1086
  //
  union PackedTexCoords
  {
    unsigned int packed;
  };

  // 1087
  //
  union PackedUnitVec
  {
    unsigned int packed;
    char array[4];
  };

  // 1088
  //
  struct GfxPackedVertex
  {
    float xyz[3];
    float binormalSign;
    GfxColor color;
    PackedTexCoords texCoord;
    PackedUnitVec normal;
    PackedUnitVec tangent;
  };

  // 1089
  //
  struct XSurfaceCollisionAabb
  {
    unsigned short mins[3];
    unsigned short maxs[3];
  };

  // 1090
  //
  struct XSurfaceCollisionNode
  {
    XSurfaceCollisionAabb aabb;
    unsigned short childBeginIndex;
    unsigned short childCount;
  };

  // 1091
  //
  struct XSurfaceCollisionLeaf
  {
    unsigned short triangleBeginIndex;
  };

  // 1092
  //
  struct XSurfaceCollisionTree
  {
    float trans[3];
    float scale[3];
    unsigned int nodeCount;
    XSurfaceCollisionNode* nodes;
    unsigned int leafCount;
    XSurfaceCollisionLeaf* leafs;
  };

  // 1093
  //
  struct XRigidVertList
  {
    unsigned short boneOffset;
    unsigned short vertCount;
    unsigned short triOffset;
    unsigned short triCount;
    XSurfaceCollisionTree* collisionTree;
  };

  // 1094
  //
  struct XSurface
  {
    char tileMode;
    bool deformed;
    unsigned short vertCount;
    unsigned short triCount;
    char zoneHandle;
    unsigned short baseTriIndex;
    unsigned short baseVertIndex;
    unsigned short* triIndices;
    XSurfaceVertexInfo vertInfo;
    GfxPackedVertex* verts0;
    unsigned int vertListCount;
    XRigidVertList* vertList;
    int partBits[6];
  };

  // 1095
  //
  struct XModelSurfs
  {
    const char* name;
    XSurface* surfs;
    unsigned short numsurfs;
    int partBits[6];
  };

  // 1096
  //
  struct XModelLodInfo
  {
    float dist;
    unsigned short numsurfs;
    unsigned short surfIndex;
    XModelSurfs* modelSurfs;
    int partBits[6];
    XSurface* surfs;
    char lod;
    char smcBaseIndexPlusOne;
    char smcSubIndexMask;
    char smcBucket;
  };

  // 1097
  //
  struct XModelCollSurf_s
  {
    XModelCollTri_s* collTris;
    int numCollTris;
    Bounds bounds;
    int boneIdx;
    int contents;
    int surfFlags;
  };

  // 1098
  //
  struct XBoneInfo
  {
    Bounds bounds;
    float radiusSquared;
  };

  // 1099
  //
  struct PhysPreset
  {
    const char* name;
    int type;
    float mass;
    float bounce;
    float friction;
    float bulletForceScale;
    float explosiveForceScale;
    const char* sndAliasPrefix;
    float piecesSpreadFraction;
    float piecesUpwardVelocity;
    bool tempDefaultToCylinder;
    bool perSurfaceSndAlias;
  };

  // 1100
  //
  struct PhysGeomInfo
  {
    BrushWrapper* brushWrapper;
    int type;
    float orientation[3][3];
    Bounds bounds;
  };

  // 1101
  //
  struct PhysCollmap
  {
    const char* name;
    unsigned int count;
    PhysGeomInfo* geoms;
    PhysMass mass;
    Bounds bounds;
  };

  // 1102
  //
  struct XModel
  {
    const char* name;
    char numBones;
    char numRootBones;
    char numsurfs;
    char lodRampType;
    float scale;
    unsigned int noScalePartBits[6];
    unsigned short* boneNames;
    char* parentList;
    short* quats;
    float* trans;
    char* partClassification;
    DObjAnimMat* baseMat;
    Material** materialHandles;
    XModelLodInfo lodInfo[4];
    char maxLoadedLod;
    char numLods;
    char collLod;
    char flags;
    XModelCollSurf_s* collSurfs;
    int numCollSurfs;
    int contents;
    XBoneInfo* boneInfo;
    float radius;
    Bounds bounds;
    int memUsage;
    bool bad;
    PhysPreset* physPreset;
    PhysCollmap* physCollmap;
  };

  // 1103
  //
  struct FxEffectDef
  {
    const char* name;
    int flags;
    int totalSize;
    int msecLoopingLife;
    int elemDefCountLooping;
    int elemDefCountOneShot;
    int elemDefCountEmission;
    FxElemDef* elemDefs;
  };

  // 1104
  //
  union FxEffectDefRef
  {
    FxEffectDef* handle;
    const char* name;
  };

  // 1105
  //
  union FxElemVisuals
  {
    const void* anonymous;
    Material* material;
    XModel* model;
    FxEffectDefRef effectDef;
    const char* soundName;
  };

  // 1106
  //
  union FxElemDefVisuals
  {
    FxElemMarkVisuals* markArray;
    FxElemVisuals* array;
    FxElemVisuals instance;
  };

  // 1107
  //
  struct FxTrailVertex
  {
    float pos[2];
    float normal[2];
    float texCoord;
  };

  // 1108
  //
  struct FxTrailDef
  {
    int scrollTimeMsec;
    int repeatDist;
    float invSplitDist;
    float invSplitArcDist;
    float invSplitTime;
    int vertCount;
    FxTrailVertex* verts;
    int indCount;
    unsigned short* inds;
  };

  // 1109
  //
  struct FxSparkFountainDef
  {
    float gravity;
    float bounceFrac;
    float bounceRand;
    float sparkSpacing;
    float sparkLength;
    int sparkCount;
    float loopTime;
    float velMin;
    float velMax;
    float velConeFrac;
    float restSpeed;
    float boostTime;
    float boostFactor;
  };

  // 1110
  //
  union FxElemExtendedDefPtr
  {
    FxTrailDef* trailDef;
    FxSparkFountainDef* sparkFountainDef;
    void* unknownDef;
  };

  // 1111
  //
  struct FxElemDef
  {
    int flags;
    FxSpawnDef spawn;
    FxFloatRange spawnRange;
    FxFloatRange fadeInRange;
    FxFloatRange fadeOutRange;
    float spawnFrustumCullRadius;
    FxIntRange spawnDelayMsec;
    FxIntRange lifeSpanMsec;
    FxFloatRange spawnOrigin[3];
    FxFloatRange spawnOffsetRadius;
    FxFloatRange spawnOffsetHeight;
    FxFloatRange spawnAngles[3];
    FxFloatRange angularVelocity[3];
    FxFloatRange initialRotation;
    FxFloatRange gravity;
    FxFloatRange reflectionFactor;
    FxElemAtlas atlas;
    char elemType;
    char visualCount;
    char velIntervalCount;
    char visStateIntervalCount;
    FxElemVelStateSample* velSamples;
    FxElemVisStateSample* visSamples;
    FxElemDefVisuals visuals;
    Bounds collBounds;
    FxEffectDefRef effectOnImpact;
    FxEffectDefRef effectOnDeath;
    FxEffectDefRef effectEmitted;
    FxFloatRange emitDist;
    FxFloatRange emitDistVariance;
    FxElemExtendedDefPtr extended;
    char sortOrder;
    char lightingFrac;
    char useItemClip;
    char fadeInfo;
  };

  // 1112
  //
  struct FxImpactEntry
  {
    FxEffectDef* nonflesh[31];
    FxEffectDef* flesh[4];
  };

  // 1113
  //
  struct MigrationPers
  {
    int time;
    bool stanceHeld;
    StanceState stance;
    StanceState stancePosition;
    int stanceTime;
    int cgameUserCmdWeapon;
    int cgameUserCmdOffHandIndex;
    unsigned int weaponSelect;
    int weaponSelectTime;
    int weaponForcedSelectTime;
    unsigned int weaponLatestPrimaryIdx;
    unsigned short primaryWeaponForAlt[1400];
    int holdBreathTime;
    int holdBreathInTime;
    int holdBreathDelay;
    float holdBreathFrac;
  };

  // 1114
  //
  struct clientUIActive_t
  {
    bool active;
    bool isRunning;
    bool cgameInitialized;
    bool cgameInitCalled;
    bool mapPreloaded;
    clientMigState_t migrationState;
    MigrationPers migrationPers;
    MigrationVerboseState verboseMigrationState;
    int verboseMigrationData;
    int keyCatchers;
    bool displayHUDWithKeycatchUI;
    connstate_t connectionState;
    bool invited;
    char itemsUnlocked[256];
    bool itemsUnlockedInited;
    bool itemsUnlockedLastGameDirty;
    unsigned short itemsUnlockedLastGame[16];
    int itemsUnlockedLastGameCount;
    char* itemsUnlockedBuffer;
    int itemsUnlockedLocalClientNum;
    int itemsUnlockedControllerIndex;
    int itemsUnlockedStatsSource;
  };

  // 1115
  //
  struct netProfilePacket_t
  {
    int iTime;
    int iSize;
    int bFragment;
  };

  // 1116
  //
  struct netProfileStream_t
  {
    netProfilePacket_t packets[60];
    int iCurrPacket;
    int iBytesPerSecond;
    int iLastBPSCalcTime;
    int iCountedPackets;
    int iCountedFragments;
    int iFragmentPercentage;
    int iLargestPacket;
    int iSmallestPacket;
  };

  // 1117
  //
  struct ClientVoicePacket_t
  {
    char data[256];
    int dataSize;
  };

  // 1118
  //
  struct voiceCommunication_t
  {
    ClientVoicePacket_t voicePackets[10];
    int voicePacketCount;
    int voicePacketLastTransmit;
    int packetsPerSec;
    int packetsPerSecStart;
  };

  // 1119
  //
  struct MaterialInfoRaw
  {
    unsigned int nameOffset;
    unsigned int refImageNameOffset;
    char gameFlags;
    char pad[1];
    char textureAtlasRowCount;
    char textureAtlasColumnCount;
    unsigned int sortKeyNameOffset;
    float maxDeformMove;
    char deformFlags;
    char usage;
    unsigned short toolFlags;
    unsigned int locale;
    unsigned short autoTexScaleWidth;
    unsigned short autoTexScaleHeight;
    float tessSize;
    int surfaceFlags;
    int contents;
  };

  // 1120
  //
  struct ProfileAtom
  {
    unsigned int value[1];
  };

  // 1121
  //
  struct ProfileWritable
  {
    int nesting;
    unsigned int hits;
    ProfileAtom start[3];
    ProfileAtom total;
    ProfileAtom child;
  };

  // 1122
  //
  struct ProfileReadable
  {
    unsigned int hits;
    ProfileAtom total;
    ProfileAtom self;
  };

  // 1123
  //
  struct profile_t
  {
    ProfileWritable write;
    ProfileReadable read;
  };

  // 1124
  //
  struct profile_guard_t
  {
    int id;
    profile_t** ppStack;
  };

  // 1125
  //
  struct ProfileStack
  {
    profile_t prof_root;
    profile_t* prof_pStack[16384];
    profile_t** prof_ppStack;
    profile_t prof_array[443];
    ProfileAtom prof_overhead_internal;
    ProfileAtom prof_overhead_inlineal;
    profile_guard_t prof_guardstack[32];
    int prof_guardpos;
    float prof_timescale;
  };

  // 1126
  //
  struct TriggerModel
  {
    int contents;
    unsigned short hullCount;
    unsigned short firstHull;
  };

  // 1127
  //
  struct FxImpactTable
  {
    const char* name;
    FxImpactEntry* table;
  };

  // 1128
  //
  union XAnimIndices
  {
    char* _1;
    unsigned short* _2;
    void* data;
  };

  // 1129
  //
  struct XAnimNotifyInfo
  {
    unsigned short name;
    float time;
  };

  // 1130
  //
  union XAnimDynamicFrames
  {
    char (*_1)[3];
    unsigned short (*_2)[3];
  };

  // 1131
  //
  union XAnimDynamicIndices
  {
    char _1[1];
    unsigned short _2[1];
  };

  // 1132
  //
  struct XAnimPartTransFrames
  {
    float mins[3];
    float size[3];
    XAnimDynamicFrames frames;
    XAnimDynamicIndices indices;
  };

  // 1133
  //
  union XAnimPartTransData
  {
    XAnimPartTransFrames frames;
    float frame0[3];
  };

  // 1134
  //
  struct XAnimPartTrans
  {
    unsigned short size;
    char smallTrans;
    XAnimPartTransData u;
  };

  // 1135
  //
  struct XAnimDeltaPartQuatDataFrames2
  {
    short (*frames)[2];
    XAnimDynamicIndices indices;
  };

  // 1136
  //
  union XAnimDeltaPartQuatData2
  {
    XAnimDeltaPartQuatDataFrames2 frames;
    short frame0[2];
  };

  // 1137
  //
  struct XAnimDeltaPartQuat2
  {
    unsigned short size;
    XAnimDeltaPartQuatData2 u;
  };

  // 1138
  //
  struct XAnimDeltaPartQuatDataFrames
  {
    short (*frames)[4];
    XAnimDynamicIndices indices;
  };

  // 1139
  //
  union XAnimDeltaPartQuatData
  {
    XAnimDeltaPartQuatDataFrames frames;
    short frame0[4];
  };

  // 1140
  //
  struct XAnimDeltaPartQuat
  {
    unsigned short size;
    XAnimDeltaPartQuatData u;
  };

  // 1141
  //
  struct XAnimDeltaPart
  {
    XAnimPartTrans* trans;
    XAnimDeltaPartQuat2* quat2;
    XAnimDeltaPartQuat* quat;
  };

  // 1142
  //
  struct XAnimParts
  {
    const char* name;
    unsigned short dataByteCount;
    unsigned short dataShortCount;
    unsigned short dataIntCount;
    unsigned short randomDataByteCount;
    unsigned short randomDataIntCount;
    unsigned short numframes;
    char flags;
    char boneCount[10];
    char notifyCount;
    char assetType;
    bool isDefault;
    unsigned int randomDataShortCount;
    unsigned int indexCount;
    float framerate;
    float frequency;
    unsigned short* names;
    char* dataByte;
    short* dataShort;
    int* dataInt;
    short* randomDataShort;
    char* randomDataByte;
    int* randomDataInt;
    XAnimIndices indices;
    XAnimNotifyInfo* notify;
    XAnimDeltaPart* deltaPart;
  };

  // 1143
  //
  struct cStaticModel_s
  {
    XModel* xmodel;
    float origin[3];
    float invScaledAxis[3][3];
    Bounds absBounds;
  };

  // 1144
  //
  struct ClipMaterial
  {
    const char* name;
    int surfaceFlags;
    int contents;
  };

  // 1145
  //
  struct cNode_t
  {
    cplane_s* plane;
    short children[2];
  };

  // 1146
  //
  struct cLeaf_t
  {
    unsigned short firstCollAabbIndex;
    unsigned short collAabbCount;
    int brushContents;
    int terrainContents;
    Bounds bounds;
    int leafBrushNode;
  };

  // 1147
  //
  struct cLeafBrushNodeLeaf_t
  {
    unsigned short* brushes;
  };

  // 1148
  //
  struct cLeafBrushNodeChildren_t
  {
    float dist;
    float range;
    unsigned short childOffset[2];
  };

  // 1149
  //
  union cLeafBrushNodeData_t
  {
    cLeafBrushNodeLeaf_t leaf;
    cLeafBrushNodeChildren_t children;
  };

  // 1150
  //
  struct cLeafBrushNode_s
  {
    char axis;
    short leafBrushCount;
    int contents;
    cLeafBrushNodeData_t data;
  };

  // 1151
  //
  struct CollisionBorder
  {
    float distEq[3];
    float zBase;
    float zSlope;
    float start;
    float length;
  };

  // 1152
  //
  struct CollisionPartition
  {
    char triCount;
    char borderCount;
    char firstVertSegment;
    int firstTri;
    CollisionBorder* borders;
  };

  // 1153
  //
  union CollisionAabbTreeIndex
  {
    int firstChildIndex;
    int partitionIndex;
  };

  // 1154
  //
  struct CollisionAabbTree
  {
    float midPoint[3];
    unsigned short materialIndex;
    unsigned short childCount;
    float halfSize[3];
    CollisionAabbTreeIndex u;
  };

  // 1155
  //
  struct cmodel_t
  {
    Bounds bounds;
    float radius;
    cLeaf_t leaf;
  };

  // 1156
  //
  struct MapTriggers
  {
    unsigned int count;
    TriggerModel* models;
    unsigned int hullCount;
    TriggerHull* hulls;
    unsigned int slabCount;
    TriggerSlab* slabs;
  };

  // 1157
  //
  struct Stage
  {
    const char* name;
    float origin[3];
    unsigned short triggerIndex;
    char sunPrimaryLightIndex;
  };

  // 1158
  //
  struct MapEnts
  {
    const char* name;
    char* entityString;
    int numEntityChars;
    MapTriggers trigger;
    Stage* stages;
    char stageCount;
  };

  // 1159
  //
  struct SModelAabbNode
  {
    Bounds bounds;
    unsigned short firstChild;
    unsigned short childCount;
  };

  // 1160
  //
  struct GfxPlacement
  {
    float quat[4];
    float origin[3];
  };

  // 1161
  //
  struct DynEntityDef
  {
    DynEntityType type;
    GfxPlacement pose;
    XModel* xModel;
    unsigned short brushModel;
    unsigned short physicsBrushModel;
    FxEffectDef* destroyFx;
    PhysPreset* physPreset;
    int health;
    PhysMass mass;
    int contents;
  };

  // 1162
  //
  struct DynEntityPose
  {
    GfxPlacement pose;
    float radius;
  };

  // 1163
  //
  struct DynEntityClient
  {
    int physObjId;
    unsigned short flags;
    unsigned short lightingHandle;
    int health;
  };

  // 1164
  //
  struct DynEntityColl
  {
    unsigned short sector;
    unsigned short nextEntInSector;
    float linkMins[2];
    float linkMaxs[2];
  };

  // 1165
  //
  struct clipMap_t
  {
    const char* name;
    int isInUse;
    int planeCount;
    cplane_s* planes;
    unsigned int numStaticModels;
    cStaticModel_s* staticModelList;
    unsigned int numMaterials;
    ClipMaterial* materials;
    unsigned int numBrushSides;
    cbrushside_t* brushsides;
    unsigned int numBrushEdges;
    char* brushEdges;
    unsigned int numNodes;
    cNode_t* nodes;
    unsigned int numLeafs;
    cLeaf_t* leafs;
    unsigned int leafbrushNodesCount;
    cLeafBrushNode_s* leafbrushNodes;
    unsigned int numLeafBrushes;
    unsigned short* leafbrushes;
    unsigned int numLeafSurfaces;
    unsigned int* leafsurfaces;
    unsigned int vertCount;
    float (*verts)[3];
    int triCount;
    unsigned short* triIndices;
    char* triEdgeIsWalkable;
    int borderCount;
    CollisionBorder* borders;
    int partitionCount;
    CollisionPartition* partitions;
    int aabbTreeCount;
    CollisionAabbTree* aabbTrees;
    unsigned int numSubModels;
    cmodel_t* cmodels;
    unsigned short numBrushes;
    cbrush_t* brushes;
    Bounds* brushBounds;
    int* brushContents;
    MapEnts* mapEnts;
    unsigned short smodelNodeCount;
    SModelAabbNode* smodelNodes;
    unsigned short dynEntCount[2];
    DynEntityDef* dynEntDefList[2];
    DynEntityPose* dynEntPoseList[2];
    DynEntityClient* dynEntClientList[2];
    DynEntityColl* dynEntCollList[2];
    unsigned int checksum;
  };

  // 1166
  //
  struct ComPrimaryLight
  {
    char type;
    char canUseShadowMap;
    char exponent;
    char unused;
    float color[3];
    float dir[3];
    float origin[3];
    float radius;
    float cosHalfFovOuter;
    float cosHalfFovInner;
    float cosHalfFovExpanded;
    float rotationLimit;
    float translationLimit;
    const char* defName;
  };

  // 1167
  //
  struct ComWorld
  {
    const char* name;
    int isInUse;
    unsigned int primaryLightCount;
    ComPrimaryLight* primaryLights;
  };

  // 1168
  //
  struct pathlink_s
  {
    float fDist;
    unsigned short nodeNum;
    char disconnectCount;
    char negotiationLink;
    char flags;
    char ubBadPlaceCount[3];
  };

  // 1170
  //
  struct pathnode_constant_t
  {
    nodeType type;
    unsigned short spawnflags;
    unsigned short targetname;
    unsigned short script_linkName;
    unsigned short script_noteworthy;
    unsigned short target;
    unsigned short animscript;
    int animscriptfunc;
    float vOrigin[3];
    float fAngle;
    float forward[2];
    float fRadius;

    union
    {
      float minUseDistSq;
      PathNodeErrorCode error;
    };

    short wOverlapNode[2];
    unsigned short totalLinkCount;
    pathlink_s* Links;
  };

  // 1171
  //
  struct pathnode_dynamic_t
  {
    void* pOwner;
    int iFreeTime;
    int iValidTime[3];
    int dangerousNodeTime[3];
    int inPlayerLOSTime;
    short wLinkCount;
    short wOverlapCount;
    short turretEntNumber;
    char userCount;
    bool hasBadPlaceLink;
  };

  // 1174
  //
  struct pathnode_transient_t
  {
    int iSearchFrame;
    pathnode_t* pNextOpen;
    pathnode_t* pPrevOpen;
    pathnode_t* pParent;
    float fCost;
    float fHeuristic;

    union
    {
      float nodeCost;
      int linkIndex;
    };
  };

  // 1172
  //
  struct pathnode_t
  {
    pathnode_constant_t constant;
    pathnode_dynamic_t dynamic;
    pathnode_transient_t transient;
  };

  // 1175
  //
  struct pathbasenode_t
  {
    float vOrigin[3];
    unsigned int type;
  };

  // 1177
  //
  struct pathnode_tree_nodes_t
  {
    int nodeCount;
    unsigned short* nodes;
  };

  // 1178
  //
  union pathnode_tree_info_t
  {
    pathnode_tree_t* child[2];
    pathnode_tree_nodes_t s;
  };

  // 1176
  //
  struct pathnode_tree_t
  {
    int axis;
    float dist;
    pathnode_tree_info_t u;
  };

  // 1179
  //
  struct PathData
  {
    unsigned int nodeCount;
    pathnode_t* nodes;
    pathbasenode_t* basenodes;
    unsigned int chainNodeCount;
    unsigned short* chainNodeForNode;
    unsigned short* nodeForChainNode;
    int visBytes;
    char* pathVis;
    int nodeTreeCount;
    pathnode_tree_t* nodeTree;
  };

  // 1180
  //
  struct VehicleTrackObstacle
  {
    float origin[2];
    float radius;
  };

  // 1181
  //
  struct VehicleTrackSector
  {
    float startEdgeDir[2];
    float startEdgeDist;
    float leftEdgeDir[2];
    float leftEdgeDist;
    float rightEdgeDir[2];
    float rightEdgeDist;
    float sectorLength;
    float sectorWidth;
    float totalPriorLength;
    float totalFollowingLength;
    VehicleTrackObstacle* obstacles;
    unsigned int obstacleCount;
  };

  // 1182
  //
  struct VehicleTrackSegment
  {
    const char* targetName;
    VehicleTrackSector* sectors;
    unsigned int sectorCount;
    VehicleTrackSegment** nextBranches;
    unsigned int nextBranchesCount;
    VehicleTrackSegment** prevBranches;
    unsigned int prevBranchesCount;
    float endEdgeDir[2];
    float endEdgeDist;
    float totalLength;
  };

  // 1183
  //
  struct VehicleTrack
  {
    VehicleTrackSegment* segments;
    unsigned int segmentCount;
  };

  // 1184
  //
  struct G_GlassPiece
  {
    unsigned short damageTaken;
    unsigned short collapseTime;
    int lastStateChangeTime;
    char impactDir;
    char impactPos[2];
  };

  // 1185
  //
  struct G_GlassName
  {
    char* nameStr;
    unsigned short name;
    unsigned short pieceCount;
    unsigned short* pieceIndices;
  };

  // 1186
  //
  struct G_GlassData
  {
    G_GlassPiece* glassPieces;
    unsigned int pieceCount;
    unsigned short damageToWeaken;
    unsigned short damageToDestroy;
    unsigned int glassNameCount;
    G_GlassName* glassNames;
    char pad[108];
  };

  // 1187
  //
  struct GameWorldSp
  {
    const char* name;
    PathData path;
    VehicleTrack vehicleTrack;
    G_GlassData* g_glassData;
  };

  // 1188
  //
  struct GameWorldMp
  {
    const char* name;
    G_GlassData* g_glassData;
  };

  // 1189
  //
  struct FxGlassDef
  {
    float halfThickness;
    float texVecs[2][2];
    GfxColor color;
    Material* material;
    Material* materialShattered;
    PhysPreset* physPreset;
  };

  // 1190
  //
  struct FxSpatialFrame
  {
    float quat[4];
    float origin[3];
  };

  // 1192
  //
  union FxGlassPiecePlace
  {
    // 1191
    //
    struct
    {
      FxSpatialFrame frame;
      float radius;
    };

    unsigned int nextFree;
  };

  // 1193
  //
  struct FxGlassPieceState
  {
    float texCoordOrigin[2];
    unsigned int supportMask;
    unsigned short initIndex;
    unsigned short geoDataStart;
    char defIndex;
    char pad[5];
    char vertCount;
    char holeDataCount;
    char crackDataCount;
    char fanDataCount;
    unsigned short flags;
    float areaX2;
  };

  // 1194
  //
  struct FxGlassPieceDynamics
  {
    int fallTime;
    int physObjId;
    int physJointId;
    float vel[3];
    float avel[3];
  };

  // 1195
  //
  struct FxGlassVertex
  {
    short x;
    short y;
  };

  // 1196
  //
  struct FxGlassHoleHeader
  {
    unsigned short uniqueVertCount;
    char touchVert;
    char pad[1];
  };

  // 1197
  //
  struct FxGlassCrackHeader
  {
    unsigned short uniqueVertCount;
    char beginVertIndex;
    char endVertIndex;
  };

  // 1198
  //
  union FxGlassGeometryData
  {
    FxGlassVertex vert;
    FxGlassHoleHeader hole;
    FxGlassCrackHeader crack;
    char asBytes[4];
    short anonymous[2];
  };

  // 1199
  //
  struct FxGlassInitPieceState
  {
    FxSpatialFrame frame;
    float radius;
    float texCoordOrigin[2];
    unsigned int supportMask;
    float areaX2;
    char defIndex;
    char vertCount;
    char fanDataCount;
    char pad[1];
  };

  // 1200
  //
  struct FxGlassSystem
  {
    int time;
    int prevTime;
    unsigned int defCount;
    unsigned int pieceLimit;
    unsigned int pieceWordCount;
    unsigned int initPieceCount;
    unsigned int cellCount;
    unsigned int activePieceCount;
    unsigned int firstFreePiece;
    unsigned int geoDataLimit;
    unsigned int geoDataCount;
    unsigned int initGeoDataCount;
    FxGlassDef* defs;
    FxGlassPiecePlace* piecePlaces;
    FxGlassPieceState* pieceStates;
    FxGlassPieceDynamics* pieceDynamics;
    FxGlassGeometryData* geoData;
    unsigned int* isInUse;
    unsigned int* cellBits;
    char* visData;
    float (*linkOrg)[3];
    float* halfThickness;
    unsigned short* lightingHandles;
    FxGlassInitPieceState* initPieceStates;
    FxGlassGeometryData* initGeoData;
    bool needToCompactData;
    char initCount;
    float effectChanceAccum;
    int lastPieceDeletionTime;
  };

  // 1201
  //
  struct FxWorld
  {
    const char* name;
    FxGlassSystem glassSys;
  };

  // 1202
  //
  struct GfxSky
  {
    int skySurfCount;
    int* skyStartSurfs;
    GfxImage* skyImage;
    char skySamplerState;
  };

  // 1203
  //
  struct GfxWorldDpvsPlanes
  {
    int cellCount;
    cplane_s* planes;
    unsigned short* nodes;
    unsigned int* sceneEntCellBits;
  };

  // 1204
  //
  struct GfxCellTreeCount
  {
    int aabbTreeCount;
  };

  // 1205
  //
  struct GfxAabbTree
  {
    Bounds bounds;
    unsigned short childCount;
    unsigned short surfaceCount;
    unsigned short startSurfIndex;
    unsigned short surfaceCountNoDecal;
    unsigned short startSurfIndexNoDecal;
    unsigned short smodelIndexCount;
    unsigned short* smodelIndexes;
    int childrenOffset;
  };

  // 1206
  //
  struct GfxCellTree
  {
    GfxAabbTree* aabbTree;
  };

  // 1208
  //
  struct GfxPortalWritable
  {
    bool isQueued;
    bool isAncestor;
    char recursionDepth;
    char hullPointCount;
    float (*hullPoints)[2];
    GfxPortal* queuedParent;
  };

  // 1209
  //
  struct DpvsPlane
  {
    float coeffs[4];
  };

  // 1207
  //
  struct GfxPortal
  {
    GfxPortalWritable writable;
    DpvsPlane plane;
    float (*vertices)[3];
    unsigned short cellIndex;
    char vertexCount;
    float hullAxis[2][3];
  };

  // 1210
  //
  struct GfxCell
  {
    Bounds bounds;
    int portalCount;
    GfxPortal* portals;
    char reflectionProbeCount;
    char* reflectionProbes;
  };

  // 1211
  //
  struct GfxReflectionProbe
  {
    float origin[3];
  };

  // 1212
  //
  struct GfxLightmapArray
  {
    GfxImage* primary;
    GfxImage* secondary;
  };

  // 1213
  //
  struct GfxWorldVertex
  {
    float xyz[3];
    float binormalSign;
    GfxColor color;
    float texCoord[2];
    float lmapCoord[2];
    PackedUnitVec normal;
    PackedUnitVec tangent;
  };

  // 1215
  //
  struct GfxWorldVertexData
  {
    GfxWorldVertex* vertices;
    IDirect3DVertexBuffer9* worldVb;
  };

  // 1216
  //
  struct GfxWorldVertexLayerData
  {
    char* data;
    IDirect3DVertexBuffer9* layerVb;
  };

  // 1217
  //
  struct GfxWorldDraw
  {
    unsigned int reflectionProbeCount;
    GfxImage** reflectionProbes;
    GfxReflectionProbe* reflectionProbeOrigins;
    GfxTexture* reflectionProbeTextures;
    int lightmapCount;
    GfxLightmapArray* lightmaps;
    GfxTexture* lightmapPrimaryTextures;
    GfxTexture* lightmapSecondaryTextures;
    GfxImage* lightmapOverridePrimary;
    GfxImage* lightmapOverrideSecondary;
    unsigned int vertexCount;
    GfxWorldVertexData vd;
    unsigned int vertexLayerDataSize;
    GfxWorldVertexLayerData vld;
    unsigned int indexCount;
    unsigned short* indices;
  };

  // 1218
  //
  struct GfxLightGridEntry
  {
    unsigned short colorsIndex;
    char primaryLightIndex;
    char needsTrace;
  };

  // 1219
  //
  struct GfxLightGridColors
  {
    char rgb[56][3];
  };

  // 1220
  //
  struct GfxLightGrid
  {
    bool hasLightRegions;
    unsigned int lastSunPrimaryLightIndex;
    unsigned short mins[3];
    unsigned short maxs[3];
    unsigned int rowAxis;
    unsigned int colAxis;
    unsigned short* rowDataStart;
    unsigned int rawRowDataSize;
    char* rawRowData;
    unsigned int entryCount;
    GfxLightGridEntry* entries;
    unsigned int colorCount;
    GfxLightGridColors* colors;
  };

  // 1221
  //
  struct GfxBrushModelWritable
  {
    Bounds bounds;
  };

  // 1222
  //
  struct GfxBrushModel
  {
    GfxBrushModelWritable writable;
    Bounds bounds;
    float radius;
    unsigned short surfaceCount;
    unsigned short startSurfIndex;
    unsigned short surfaceCountNoDecal;
  };

  // 1223
  //
  struct MaterialMemory
  {
    Material* material;
    int memory;
  };

  // 1224
  //
  struct sunflare_t
  {
    bool hasValidData;
    Material* spriteMaterial;
    Material* flareMaterial;
    float spriteSize;
    float flareMinSize;
    float flareMinDot;
    float flareMaxSize;
    float flareMaxDot;
    float flareMaxAlpha;
    int flareFadeInTime;
    int flareFadeOutTime;
    float blindMinDot;
    float blindMaxDot;
    float blindMaxDarken;
    int blindFadeInTime;
    int blindFadeOutTime;
    float glareMinDot;
    float glareMaxDot;
    float glareMaxLighten;
    int glareFadeInTime;
    int glareFadeOutTime;
    float sunFxPosition[3];
  };

  // 1225
  //
  struct XModelDrawInfo
  {
    char hasGfxEntIndex;
    char lod;
    unsigned short surfId;
  };

  // 1226
  //
  struct GfxSceneDynModel
  {
    XModelDrawInfo info;
    unsigned short dynEntId;
  };

  // 1227
  //
  struct BModelDrawInfo
  {
    unsigned short surfId;
  };

  // 1228
  //
  struct GfxSceneDynBrush
  {
    BModelDrawInfo info;
    unsigned short dynEntId;
  };

  // 1229
  //
  struct GfxShadowGeometry
  {
    unsigned short surfaceCount;
    unsigned short smodelCount;
    unsigned short* sortedSurfIndex;
    unsigned short* smodelIndex;
  };

  // 1230
  //
  struct GfxLightRegionAxis
  {
    float dir[3];
    float midPoint;
    float halfSize;
  };

  // 1231
  //
  struct GfxLightRegionHull
  {
    float kdopMidPoint[9];
    float kdopHalfSize[9];
    unsigned int axisCount;
    GfxLightRegionAxis* axis;
  };

  // 1232
  //
  struct GfxLightRegion
  {
    unsigned int hullCount;
    GfxLightRegionHull* hulls;
  };

  // 1233
  //
  struct GfxStaticModelInst
  {
    Bounds bounds;
    float lightingOrigin[3];
  };

  // 1234
  //
  struct srfTriangles_t
  {
    unsigned int vertexLayerData;
    unsigned int firstVertex;
    unsigned short vertexCount;
    unsigned short triCount;
    unsigned int baseIndex;
  };

  // 1235
  //
  struct GfxSurfaceLightingAndFlagsFields
  {
    char lightmapIndex;
    char reflectionProbeIndex;
    char primaryLightIndex;
    char flags;
  };

  // 1236
  //
  union GfxSurfaceLightingAndFlags
  {
    GfxSurfaceLightingAndFlagsFields fields;
    unsigned int packed;
  };

  // 1237
  //
  struct GfxSurface
  {
    srfTriangles_t tris;
    Material* material;
    GfxSurfaceLightingAndFlags laf;
  };

  // 1238
  //
  struct GfxSurfaceBounds
  {
    Bounds bounds;
  };

  // 1239
  //
  struct GfxPackedPlacement
  {
    float origin[3];
    float axis[3][3];
    float scale;
  };

  // 1240
  //
  struct GfxStaticModelDrawInst
  {
    GfxPackedPlacement placement;
    XModel* model;
    unsigned short cullDist;
    unsigned short lightingHandle;
    char reflectionProbeIndex;
    char primaryLightIndex;
    char flags;
    char firstMtlSkinIndex;
    GfxColor groundLighting;
    unsigned short cacheId[4];
  };

  // 1241
  //
  struct GfxWorldDpvsStatic
  {
    unsigned int smodelCount;
    unsigned int staticSurfaceCount;
    unsigned int staticSurfaceCountNoDecal;
    unsigned int litOpaqueSurfsBegin;
    unsigned int litOpaqueSurfsEnd;
    unsigned int litTransSurfsBegin;
    unsigned int litTransSurfsEnd;
    unsigned int shadowCasterSurfsBegin;
    unsigned int shadowCasterSurfsEnd;
    unsigned int emissiveSurfsBegin;
    unsigned int emissiveSurfsEnd;
    unsigned int smodelVisDataCount;
    unsigned int surfaceVisDataCount;
    char* smodelVisData[3];
    char* surfaceVisData[3];
    unsigned short* sortedSurfIndex;
    GfxStaticModelInst* smodelInsts;
    GfxSurface* surfaces;
    GfxSurfaceBounds* surfacesBounds;
    GfxStaticModelDrawInst* smodelDrawInsts;
    GfxDrawSurf* surfaceMaterials;
    unsigned int* surfaceCastsSunShadow;
    volatile int usageCount;
  };

  // 1242
  //
  struct GfxWorldDpvsDynamic
  {
    unsigned int dynEntClientWordCount[2];
    unsigned int dynEntClientCount[2];
    unsigned int* dynEntCellBits[2];
    char* dynEntVisData[2][3];
  };

  // 1243
  //
  struct GfxHeroOnlyLight
  {
    char type;
    char unused[3];
    float color[3];
    float dir[3];
    float origin[3];
    float radius;
    float cosHalfFovOuter;
    float cosHalfFovInner;
    int exponent;
  };

  // 1244
  //
  struct GfxWorld
  {
    const char* name;
    const char* baseName;
    int planeCount;
    int nodeCount;
    unsigned int surfaceCount;
    int skyCount;
    GfxSky* skies;
    unsigned int lastSunPrimaryLightIndex;
    unsigned int primaryLightCount;
    unsigned int sortKeyLitDecal;
    unsigned int sortKeyEffectDecal;
    unsigned int sortKeyEffectAuto;
    unsigned int sortKeyDistortion;
    GfxWorldDpvsPlanes dpvsPlanes;
    GfxCellTreeCount* aabbTreeCounts;
    GfxCellTree* aabbTrees;
    GfxCell* cells;
    GfxWorldDraw draw;
    GfxLightGrid lightGrid;
    int modelCount;
    GfxBrushModel* models;
    Bounds bounds;
    unsigned int checksum;
    int materialMemoryCount;
    MaterialMemory* materialMemory;
    sunflare_t sun;
    float outdoorLookupMatrix[4][4];
    GfxImage* outdoorImage;
    unsigned int* cellCasterBits;
    unsigned int* cellHasSunLitSurfsBits;
    GfxSceneDynModel* sceneDynModel;
    GfxSceneDynBrush* sceneDynBrush;
    unsigned int* primaryLightEntityShadowVis;
    unsigned int* primaryLightDynEntShadowVis[2];
    char* nonSunPrimaryLightForModelDynEnt;
    GfxShadowGeometry* shadowGeom;
    GfxLightRegion* lightRegion;
    GfxWorldDpvsStatic dpvs;
    GfxWorldDpvsDynamic dpvsDyn;
    unsigned int mapVtxChecksum;
    unsigned int heroOnlyLightCount;
    GfxHeroOnlyLight* heroOnlyLights;
    char fogTypesAllowed;
  };

  // 1245
  //
  struct GfxLightImage
  {
    GfxImage* image;
    char samplerState;
  };

  // 1246
  //
  struct GfxLightDef
  {
    const char* name;
    GfxLightImage attenuation;
    int lmapLookupStart;
  };

  // 1247
  //
  struct rectDef_s
  {
    float x;
    float y;
    float w;
    float h;
    char horzAlign;
    char vertAlign;
  };

  // 1248
  //
  struct windowDef_t
  {
    const char* name;
    rectDef_s rect;
    rectDef_s rectClient;
    const char* group;
    int style;
    int border;
    int ownerDraw;
    int ownerDrawFlags;
    float borderSize;
    int staticFlags;
    int dynamicFlags[1];
    int nextTime;
    float foreColor[4];
    float backColor[4];
    float borderColor[4];
    float outlineColor[4];
    float disableColor[4];
    Material* background;
  };

  // 1249
  //
  struct MenuEventHandlerSet
  {
    int eventHandlerCount;
    MenuEventHandler** eventHandlers;
  };

  // 1250
  //
  struct ExpressionString
  {
    const char* string;
  };

  // 1252
  //
  union operandInternalDataUnion
  {
    int intVal;
    float floatVal;
    ExpressionString stringVal;
    Statement_s* function;
  };

  // 1253
  //
  struct Operand
  {
    expDataType dataType;
    operandInternalDataUnion internals;
  };

  // 1251
  //
  struct Statement_s
  {
    int numEntries;
    expressionEntry* entries;
    ExpressionSupportingData* supportingData;
    int lastExecuteTime;
    Operand lastResult;
  };

  // 1254
  //
  union entryInternalData
  {
    int op;
    Operand operand;
  };

  // 1255
  //
  struct expressionEntry
  {
    int type;
    entryInternalData data;
  };

  // 1256
  //
  struct UIFunctionList
  {
    int totalFunctions;
    Statement_s** functions;
  };

  // 1257
  //
  struct StaticDvar
  {
    dvar_t* dvar;
    char* dvarName;
  };

  // 1258
  //
  struct StaticDvarList
  {
    int numStaticDvars;
    StaticDvar** staticDvars;
  };

  // 1259
  //
  struct StringList
  {
    int totalStrings;
    const char** strings;
  };

  // 1260
  //
  struct ExpressionSupportingData
  {
    UIFunctionList uifunctions;
    StaticDvarList staticDvarList;
    StringList uiStrings;
  };

  // 1261
  //
  struct ConditionalScript
  {
    MenuEventHandlerSet* eventHandlerSet;
    Statement_s* eventExpression;
  };

  // 1262
  //
  struct SetLocalVarData
  {
    const char* localVarName;
    Statement_s* expression;
  };

  // 1263
  //
  union EventData
  {
    const char* unconditionalScript;
    ConditionalScript* conditionalScript;
    MenuEventHandlerSet* elseScript;
    SetLocalVarData* setLocalVarData;
  };

  // 1264
  //
  struct MenuEventHandler
  {
    EventData eventData;
    char eventType;
  };

  // 1265
  //
  struct ItemKeyHandler
  {
    int key;
    MenuEventHandlerSet* action;
    ItemKeyHandler* next;
  };

  // 1276
  //
  struct menuTransition
  {
    int transitionType;
    int targetField;
    int startTime;
    float startVal;
    float endVal;
    float time;
    int endTriggerType;
  };

  // 1266
  //
  struct menuDef_t
  {
    windowDef_t window;
    const char* font;
    int fullScreen;
    int itemCount;
    int fontIndex;
    int cursorItem[1];
    int fadeCycle;
    float fadeClamp;
    float fadeAmount;
    float fadeInAmount;
    float blurRadius;
    MenuEventHandlerSet* onOpen;
    MenuEventHandlerSet* onCloseRequest;
    MenuEventHandlerSet* onClose;
    MenuEventHandlerSet* onESC;
    ItemKeyHandler* onKey;
    Statement_s* visibleExp;
    const char* allowedBinding;
    const char* soundName;
    int imageTrack;
    float focusColor[4];
    Statement_s* rectXExp;
    Statement_s* rectYExp;
    Statement_s* rectWExp;
    Statement_s* rectHExp;
    Statement_s* openSoundExp;
    Statement_s* closeSoundExp;
    itemDef_s** items;
    menuTransition scaleTransition[1];
    menuTransition alphaTransition[1];
    menuTransition xTransition[1];
    menuTransition yTransition[1];
    ExpressionSupportingData* expressionData;
  };

  // 1267
  //
  struct columnInfo_s
  {
    int pos;
    int width;
    int maxChars;
    int alignment;
  };

  // 1268
  //
  struct listBoxDef_s
  {
    int mousePos;
    int startPos[1];
    int endPos[1];
    int drawPadding;
    float elementWidth;
    float elementHeight;
    int elementStyle;
    int numColumns;
    columnInfo_s columnInfo[16];
    MenuEventHandlerSet* onDoubleClick;
    int notselectable;
    int noScrollBars;
    int usePaging;
    float selectBorder[4];
    Material* selectIcon;
  };

  // 1269
  //
  struct editFieldDef_s
  {
    float minVal;
    float maxVal;
    float defVal;
    float range;
    int maxChars;
    int maxCharsGotoNext;
    int maxPaintChars;
    int paintOffset;
  };

  // 1270
  //
  struct multiDef_s
  {
    const char* dvarList[32];
    const char* dvarStr[32];
    float dvarValue[32];
    int count;
    int strDef;
  };

  // 1271
  //
  struct newsTickerDef_s
  {
    int feedId;
    int speed;
    int spacing;
    int lastTime;
    int start;
    int end;
    float x;
  };

  // 1272
  //
  struct textScrollDef_s
  {
    int startTime;
  };

  // 1273
  //
  union itemDefData_t
  {
    listBoxDef_s* listBox;
    editFieldDef_s* editField;
    multiDef_s* multi;
    const char* enumDvarName;
    newsTickerDef_s* ticker;
    textScrollDef_s* scroll;
    void* data;
  };

  // 1274
  //
  struct ItemFloatExpression
  {
    int target;
    Statement_s* expression;
  };

  // 1275
  //
  struct itemDef_s
  {
    windowDef_t window;
    rectDef_s textRect[1];
    int type;
    int dataType;
    int alignment;
    int fontEnum;
    int textAlignMode;
    float textalignx;
    float textaligny;
    float textscale;
    int textStyle;
    int gameMsgWindowIndex;
    int gameMsgWindowMode;
    const char* text;
    int itemFlags;
    menuDef_t* parent;
    MenuEventHandlerSet* mouseEnterText;
    MenuEventHandlerSet* mouseExitText;
    MenuEventHandlerSet* mouseEnter;
    MenuEventHandlerSet* mouseExit;
    MenuEventHandlerSet* action;
    MenuEventHandlerSet* accept;
    MenuEventHandlerSet* onFocus;
    MenuEventHandlerSet* leaveFocus;
    const char* dvar;
    const char* dvarTest;
    ItemKeyHandler* onKey;
    const char* enableDvar;
    const char* localVar;
    int dvarFlags;
    snd_alias_list_t* focusSound;
    float special;
    int cursorPos[1];
    itemDefData_t typeData;
    int imageTrack;
    int floatExpressionCount;
    ItemFloatExpression* floatExpressions;
    Statement_s* visibleExp;
    Statement_s* disabledExp;
    Statement_s* textExp;
    Statement_s* materialExp;
    float glowColor[4];
    bool decayActive;
    int fxBirthTime;
    int fxLetterTime;
    int fxDecayStartTime;
    int fxDecayDuration;
    int lastSoundPlayedTime;
  };

  // 1277
  //
  struct MenuList
  {
    const char* name;
    int menuCount;
    menuDef_t** menus;
  };

  // 1278
  //
  struct LocalizeEntry
  {
    const char* value;
    const char* name;
  };

  // 1279
  //
  struct TracerDef
  {
    const char* name;
    Material* material;
    unsigned int drawInterval;
    float speed;
    float beamLength;
    float beamWidth;
    float screwRadius;
    float screwDist;
    float colors[5][4];
  };

  // 1280
  //
  struct WeaponDef
  {
    const char* szOverlayName;
    XModel** gunXModel;
    XModel* handXModel;
    const char** szXAnimsRightHanded;
    const char** szXAnimsLeftHanded;
    const char* szModeName;
    unsigned short* notetrackSoundMapKeys;
    unsigned short* notetrackSoundMapValues;
    unsigned short* notetrackRumbleMapKeys;
    unsigned short* notetrackRumbleMapValues;
    int playerAnimType;
    weapType_t weapType;
    weapClass_t weapClass;
    PenetrateType penetrateType;
    weapInventoryType_t inventoryType;
    weapFireType_t fireType;
    OffhandClass offhandClass;
    weapStance_t stance;
    FxEffectDef* viewFlashEffect;
    FxEffectDef* worldFlashEffect;
    snd_alias_list_t* pickupSound;
    snd_alias_list_t* pickupSoundPlayer;
    snd_alias_list_t* ammoPickupSound;
    snd_alias_list_t* ammoPickupSoundPlayer;
    snd_alias_list_t* projectileSound;
    snd_alias_list_t* pullbackSound;
    snd_alias_list_t* pullbackSoundPlayer;
    snd_alias_list_t* fireSound;
    snd_alias_list_t* fireSoundPlayer;
    snd_alias_list_t* fireSoundPlayerAkimbo;
    snd_alias_list_t* fireLoopSound;
    snd_alias_list_t* fireLoopSoundPlayer;
    snd_alias_list_t* fireStopSound;
    snd_alias_list_t* fireStopSoundPlayer;
    snd_alias_list_t* fireLastSound;
    snd_alias_list_t* fireLastSoundPlayer;
    snd_alias_list_t* emptyFireSound;
    snd_alias_list_t* emptyFireSoundPlayer;
    snd_alias_list_t* meleeSwipeSound;
    snd_alias_list_t* meleeSwipeSoundPlayer;
    snd_alias_list_t* meleeHitSound;
    snd_alias_list_t* meleeMissSound;
    snd_alias_list_t* rechamberSound;
    snd_alias_list_t* rechamberSoundPlayer;
    snd_alias_list_t* reloadSound;
    snd_alias_list_t* reloadSoundPlayer;
    snd_alias_list_t* reloadEmptySound;
    snd_alias_list_t* reloadEmptySoundPlayer;
    snd_alias_list_t* reloadStartSound;
    snd_alias_list_t* reloadStartSoundPlayer;
    snd_alias_list_t* reloadEndSound;
    snd_alias_list_t* reloadEndSoundPlayer;
    snd_alias_list_t* detonateSound;
    snd_alias_list_t* detonateSoundPlayer;
    snd_alias_list_t* nightVisionWearSound;
    snd_alias_list_t* nightVisionWearSoundPlayer;
    snd_alias_list_t* nightVisionRemoveSound;
    snd_alias_list_t* nightVisionRemoveSoundPlayer;
    snd_alias_list_t* altSwitchSound;
    snd_alias_list_t* altSwitchSoundPlayer;
    snd_alias_list_t* raiseSound;
    snd_alias_list_t* raiseSoundPlayer;
    snd_alias_list_t* firstRaiseSound;
    snd_alias_list_t* firstRaiseSoundPlayer;
    snd_alias_list_t* putawaySound;
    snd_alias_list_t* putawaySoundPlayer;
    snd_alias_list_t* scanSound;
    snd_alias_list_t** bounceSound;
    FxEffectDef* viewShellEjectEffect;
    FxEffectDef* worldShellEjectEffect;
    FxEffectDef* viewLastShotEjectEffect;
    FxEffectDef* worldLastShotEjectEffect;
    Material* reticleCenter;
    Material* reticleSide;
    int iReticleCenterSize;
    int iReticleSideSize;
    int iReticleMinOfs;
    activeReticleType_t activeReticleType;
    float vStandMove[3];
    float vStandRot[3];
    float strafeMove[3];
    float strafeRot[3];
    float vDuckedOfs[3];
    float vDuckedMove[3];
    float vDuckedRot[3];
    float vProneOfs[3];
    float vProneMove[3];
    float vProneRot[3];
    float fPosMoveRate;
    float fPosProneMoveRate;
    float fStandMoveMinSpeed;
    float fDuckedMoveMinSpeed;
    float fProneMoveMinSpeed;
    float fPosRotRate;
    float fPosProneRotRate;
    float fStandRotMinSpeed;
    float fDuckedRotMinSpeed;
    float fProneRotMinSpeed;
    XModel** worldModel;
    XModel* worldClipModel;
    XModel* rocketModel;
    XModel* knifeModel;
    XModel* worldKnifeModel;
    Material* hudIcon;
    weaponIconRatioType_t hudIconRatio;
    Material* pickupIcon;
    weaponIconRatioType_t pickupIconRatio;
    Material* ammoCounterIcon;
    weaponIconRatioType_t ammoCounterIconRatio;
    ammoCounterClipType_t ammoCounterClip;
    int iStartAmmo;
    const char* szAmmoName;
    int iAmmoIndex;
    const char* szClipName;
    int iClipIndex;
    int iMaxAmmo;
    int shotCount;
    const char* szSharedAmmoCapName;
    int iSharedAmmoCapIndex;
    int iSharedAmmoCap;
    int damage;
    int playerDamage;
    int iMeleeDamage;
    int iDamageType;
    int iFireDelay;
    int iMeleeDelay;
    int meleeChargeDelay;
    int iDetonateDelay;
    int iRechamberTime;
    int rechamberTimeOneHanded;
    int iRechamberBoltTime;
    int iHoldFireTime;
    int iDetonateTime;
    int iMeleeTime;
    int meleeChargeTime;
    int iReloadTime;
    int reloadShowRocketTime;
    int iReloadEmptyTime;
    int iReloadAddTime;
    int iReloadStartTime;
    int iReloadStartAddTime;
    int iReloadEndTime;
    int iDropTime;
    int iRaiseTime;
    int iAltDropTime;
    int quickDropTime;
    int quickRaiseTime;
    int iBreachRaiseTime;
    int iEmptyRaiseTime;
    int iEmptyDropTime;
    int sprintInTime;
    int sprintLoopTime;
    int sprintOutTime;
    int stunnedTimeBegin;
    int stunnedTimeLoop;
    int stunnedTimeEnd;
    int nightVisionWearTime;
    int nightVisionWearTimeFadeOutEnd;
    int nightVisionWearTimePowerUp;
    int nightVisionRemoveTime;
    int nightVisionRemoveTimePowerDown;
    int nightVisionRemoveTimeFadeInStart;
    int fuseTime;
    int aiFuseTime;
    float autoAimRange;
    float aimAssistRange;
    float aimAssistRangeAds;
    float aimPadding;
    float enemyCrosshairRange;
    float moveSpeedScale;
    float adsMoveSpeedScale;
    float sprintDurationScale;
    float fAdsZoomInFrac;
    float fAdsZoomOutFrac;
    Material* overlayMaterial;
    Material* overlayMaterialLowRes;
    Material* overlayMaterialEMP;
    Material* overlayMaterialEMPLowRes;
    weapOverlayReticle_t overlayReticle;
    WeapOverlayInteface_t overlayInterface;
    float overlayWidth;
    float overlayHeight;
    float overlayWidthSplitscreen;
    float overlayHeightSplitscreen;
    float fAdsBobFactor;
    float fAdsViewBobMult;
    float fHipSpreadStandMin;
    float fHipSpreadDuckedMin;
    float fHipSpreadProneMin;
    float hipSpreadStandMax;
    float hipSpreadDuckedMax;
    float hipSpreadProneMax;
    float fHipSpreadDecayRate;
    float fHipSpreadFireAdd;
    float fHipSpreadTurnAdd;
    float fHipSpreadMoveAdd;
    float fHipSpreadDuckedDecay;
    float fHipSpreadProneDecay;
    float fHipReticleSidePos;
    float fAdsIdleAmount;
    float fHipIdleAmount;
    float adsIdleSpeed;
    float hipIdleSpeed;
    float fIdleCrouchFactor;
    float fIdleProneFactor;
    float fGunMaxPitch;
    float fGunMaxYaw;
    float swayMaxAngle;
    float swayLerpSpeed;
    float swayPitchScale;
    float swayYawScale;
    float swayHorizScale;
    float swayVertScale;
    float swayShellShockScale;
    float adsSwayMaxAngle;
    float adsSwayLerpSpeed;
    float adsSwayPitchScale;
    float adsSwayYawScale;
    float adsSwayHorizScale;
    float adsSwayVertScale;
    float adsViewErrorMin;
    float adsViewErrorMax;
    PhysCollmap* physCollmap;
    float dualWieldViewModelOffset;
    weaponIconRatioType_t killIconRatio;
    int iReloadAmmoAdd;
    int iReloadStartAdd;
    int ammoDropStockMin;
    int ammoDropClipPercentMin;
    int ammoDropClipPercentMax;
    int iExplosionRadius;
    int iExplosionRadiusMin;
    int iExplosionInnerDamage;
    int iExplosionOuterDamage;
    float damageConeAngle;
    float bulletExplDmgMult;
    float bulletExplRadiusMult;
    int iProjectileSpeed;
    int iProjectileSpeedUp;
    int iProjectileSpeedForward;
    int iProjectileActivateDist;
    float projLifetime;
    float timeToAccelerate;
    float projectileCurvature;
    XModel* projectileModel;
    weapProjExposion_t projExplosion;
    FxEffectDef* projExplosionEffect;
    FxEffectDef* projDudEffect;
    snd_alias_list_t* projExplosionSound;
    snd_alias_list_t* projDudSound;
    WeapStickinessType stickiness;
    float lowAmmoWarningThreshold;
    float ricochetChance;
    float* parallelBounce;
    float* perpendicularBounce;
    FxEffectDef* projTrailEffect;
    FxEffectDef* projBeaconEffect;
    float vProjectileColor[3];
    guidedMissileType_t guidedMissileType;
    float maxSteeringAccel;
    int projIgnitionDelay;
    FxEffectDef* projIgnitionEffect;
    snd_alias_list_t* projIgnitionSound;
    float fAdsAimPitch;
    float fAdsCrosshairInFrac;
    float fAdsCrosshairOutFrac;
    int adsGunKickReducedKickBullets;
    float adsGunKickReducedKickPercent;
    float fAdsGunKickPitchMin;
    float fAdsGunKickPitchMax;
    float fAdsGunKickYawMin;
    float fAdsGunKickYawMax;
    float fAdsGunKickAccel;
    float fAdsGunKickSpeedMax;
    float fAdsGunKickSpeedDecay;
    float fAdsGunKickStaticDecay;
    float fAdsViewKickPitchMin;
    float fAdsViewKickPitchMax;
    float fAdsViewKickYawMin;
    float fAdsViewKickYawMax;
    float fAdsViewScatterMin;
    float fAdsViewScatterMax;
    float fAdsSpread;
    int hipGunKickReducedKickBullets;
    float hipGunKickReducedKickPercent;
    float fHipGunKickPitchMin;
    float fHipGunKickPitchMax;
    float fHipGunKickYawMin;
    float fHipGunKickYawMax;
    float fHipGunKickAccel;
    float fHipGunKickSpeedMax;
    float fHipGunKickSpeedDecay;
    float fHipGunKickStaticDecay;
    float fHipViewKickPitchMin;
    float fHipViewKickPitchMax;
    float fHipViewKickYawMin;
    float fHipViewKickYawMax;
    float fHipViewScatterMin;
    float fHipViewScatterMax;
    float fightDist;
    float maxDist;
    const char* accuracyGraphName[2];
    float (*originalAccuracyGraphKnots[2])[2];
    unsigned short originalAccuracyGraphKnotCount[2];
    int iPositionReloadTransTime;
    float leftArc;
    float rightArc;
    float topArc;
    float bottomArc;
    float accuracy;
    float aiSpread;
    float playerSpread;
    float minTurnSpeed[2];
    float maxTurnSpeed[2];
    float pitchConvergenceTime;
    float yawConvergenceTime;
    float suppressTime;
    float maxRange;
    float fAnimHorRotateInc;
    float fPlayerPositionDist;
    const char* szUseHintString;
    const char* dropHintString;
    int iUseHintStringIndex;
    int dropHintStringIndex;
    float horizViewJitter;
    float vertViewJitter;
    float scanSpeed;
    float scanAccel;
    int scanPauseTime;
    const char* szScript;
    float fOOPosAnimLength[2];
    int minDamage;
    int minPlayerDamage;
    float fMaxDamageRange;
    float fMinDamageRange;
    float destabilizationRateTime;
    float destabilizationCurvatureMax;
    int destabilizeDistance;
    float* locationDamageMultipliers;
    const char* fireRumble;
    const char* meleeImpactRumble;
    TracerDef* tracerType;
    float turretScopeZoomRate;
    float turretScopeZoomMin;
    float turretScopeZoomMax;
    float turretOverheatUpRate;
    float turretOverheatDownRate;
    float turretOverheatPenalty;
    snd_alias_list_t* turretOverheatSound;
    FxEffectDef* turretOverheatEffect;
    const char* turretBarrelSpinRumble;
    float turretBarrelSpinSpeed;
    float turretBarrelSpinUpTime;
    float turretBarrelSpinDownTime;
    snd_alias_list_t* turretBarrelSpinMaxSnd;
    snd_alias_list_t* turretBarrelSpinUpSnd[4];
    snd_alias_list_t* turretBarrelSpinDownSnd[4];
    snd_alias_list_t* missileConeSoundAlias;
    snd_alias_list_t* missileConeSoundAliasAtBase;
    float missileConeSoundRadiusAtTop;
    float missileConeSoundRadiusAtBase;
    float missileConeSoundHeight;
    float missileConeSoundOriginOffset;
    float missileConeSoundVolumescaleAtCore;
    float missileConeSoundVolumescaleAtEdge;
    float missileConeSoundVolumescaleCoreSize;
    float missileConeSoundPitchAtTop;
    float missileConeSoundPitchAtBottom;
    float missileConeSoundPitchTopSize;
    float missileConeSoundPitchBottomSize;
    float missileConeSoundCrossfadeTopSize;
    float missileConeSoundCrossfadeBottomSize;
    bool sharedAmmo;
    bool lockonSupported;
    bool requireLockonToFire;
    bool bigExplosion;
    bool noAdsWhenMagEmpty;
    bool avoidDropCleanup;
    bool inheritsPerks;
    bool crosshairColorChange;
    bool bRifleBullet;
    bool armorPiercing;
    bool bBoltAction;
    bool aimDownSight;
    bool bRechamberWhileAds;
    bool bBulletExplosiveDamage;
    bool bCookOffHold;
    bool bClipOnly;
    bool noAmmoPickup;
    bool adsFireOnly;
    bool cancelAutoHolsterWhenEmpty;
    bool disableSwitchToWhenEmpty;
    bool suppressAmmoReserveDisplay;
    bool laserSightDuringNightvision;
    bool markableViewmodel;
    bool noDualWield;
    bool flipKillIcon;
    bool bNoPartialReload;
    bool bSegmentedReload;
    bool blocksProne;
    bool silenced;
    bool isRollingGrenade;
    bool projExplosionEffectForceNormalUp;
    bool bProjImpactExplode;
    bool stickToPlayers;
    bool hasDetonator;
    bool disableFiring;
    bool timedDetonation;
    bool rotate;
    bool holdButtonToThrow;
    bool freezeMovementWhenFiring;
    bool thermalScope;
    bool altModeSameWeapon;
    bool turretBarrelSpinEnabled;
    bool missileConeSoundEnabled;
    bool missileConeSoundPitchshiftEnabled;
    bool missileConeSoundCrossfadeEnabled;
    bool offhandHoldIsCancelable;
  };

  // 1281
  //
  struct WeaponCompleteDef
  {
    const char* szInternalName;
    WeaponDef* weapDef;
    const char* szDisplayName;
    unsigned short* hideTags;
    const char** szXAnims;
    float fAdsZoomFov;
    int iAdsTransInTime;
    int iAdsTransOutTime;
    int iClipSize;
    ImpactType impactType;
    int iFireTime;
    weaponIconRatioType_t dpadIconRatio;
    float penetrateMultiplier;
    float fAdsViewKickCenterSpeed;
    float fHipViewKickCenterSpeed;
    const char* szAltWeaponName;
    unsigned int altWeaponIndex;
    int iAltRaiseTime;
    Material* killIcon;
    Material* dpadIcon;
    int fireAnimLength;
    int iFirstRaiseTime;
    int ammoDropStockMax;
    float adsDofStart;
    float adsDofEnd;
    unsigned short accuracyGraphKnotCount[2];
    float (*accuracyGraphKnots[2])[2];
    bool motionTracker;
    bool enhanced;
    bool dpadIconShowsAmmo;
  };

  // 1282
  //
  struct SndDriverGlobals
  {
    const char* name;
  };

  // 1283
  //
  struct RawFile
  {
    const char* name;
    int compressedLen;
    int len;
    const char* buffer;
  };

  // 1284
  //
  struct StringTableCell
  {
    const char* string;
    int hash;
  };

  // 1285
  //
  struct StringTable
  {
    const char* name;
    int columnCount;
    int rowCount;
    StringTableCell* values;
  };

  // 1286
  //
  struct LbColumnDef
  {
    const char* name;
    int id;
    int propertyId;
    bool hidden;
    const char* statName;
    LbColType type;
    int precision;
    LbAggType agg;
  };

  // 1287
  //
  struct LeaderboardDef
  {
    const char* name;
    int id;
    int columnCount;
    int xpColId;
    int prestigeColId;
    LbColumnDef* columns;
  };

  // 1288
  //
  struct StructuredDataEnumEntry
  {
    const char* string;
    unsigned short index;
  };

  // 1289
  //
  struct StructuredDataEnum
  {
    int entryCount;
    int reservedEntryCount;
    StructuredDataEnumEntry* entries;
  };

  // 1290
  //
  union StructuredDataTypeUnion
  {
    unsigned int stringDataLength;
    int enumIndex;
    int structIndex;
    int indexedArrayIndex;
    int enumedArrayIndex;
  };

  // 1291
  //
  struct StructuredDataType
  {
    StructuredDataTypeCategory type;
    StructuredDataTypeUnion u;
  };

  // 1292
  //
  struct StructuredDataStructProperty
  {
    const char* name;
    StructuredDataType type;
    unsigned int offset;
  };

  // 1293
  //
  struct StructuredDataStruct
  {
    int propertyCount;
    StructuredDataStructProperty* properties;
    int size;
    unsigned int bitOffset;
  };

  // 1294
  //
  struct StructuredDataIndexedArray
  {
    int arraySize;
    StructuredDataType elementType;
    unsigned int elementSize;
  };

  // 1295
  //
  struct StructuredDataEnumedArray
  {
    int enumIndex;
    StructuredDataType elementType;
    unsigned int elementSize;
  };

  // 1296
  //
  struct StructuredDataDef
  {
    int version;
    unsigned int formatChecksum;
    int enumCount;
    StructuredDataEnum* enums;
    int structCount;
    StructuredDataStruct* structs;
    int indexedArrayCount;
    StructuredDataIndexedArray* indexedArrays;
    int enumedArrayCount;
    StructuredDataEnumedArray* enumedArrays;
    StructuredDataType rootType;
    unsigned int size;
  };

  // 1297
  //
  struct StructuredDataDefSet
  {
    const char* name;
    unsigned int defCount;
    StructuredDataDef* defs;
  };

  // 1298
  //
  struct VehiclePhysDef
  {
    int physicsEnabled;
    const char* physPresetName;
    PhysPreset* physPreset;
    const char* accelGraphName;
    VehicleAxleType steeringAxle;
    VehicleAxleType powerAxle;
    VehicleAxleType brakingAxle;
    float topSpeed;
    float reverseSpeed;
    float maxVelocity;
    float maxPitch;
    float maxRoll;
    float suspensionTravelFront;
    float suspensionTravelRear;
    float suspensionStrengthFront;
    float suspensionDampingFront;
    float suspensionStrengthRear;
    float suspensionDampingRear;
    float frictionBraking;
    float frictionCoasting;
    float frictionTopSpeed;
    float frictionSide;
    float frictionSideRear;
    float velocityDependentSlip;
    float rollStability;
    float rollResistance;
    float pitchResistance;
    float yawResistance;
    float uprightStrengthPitch;
    float uprightStrengthRoll;
    float targetAirPitch;
    float airYawTorque;
    float airPitchTorque;
    float minimumMomentumForCollision;
    float collisionLaunchForceScale;
    float wreckedMassScale;
    float wreckedBodyFriction;
    float minimumJoltForNotify;
    float slipThresholdFront;
    float slipThresholdRear;
    float slipFricScaleFront;
    float slipFricScaleRear;
    float slipFricRateFront;
    float slipFricRateRear;
    float slipYawTorque;
  };

  // 1299
  //
  struct VehicleDef
  {
    const char* name;
    VehicleType type;
    const char* useHintString;
    int health;
    int quadBarrel;
    float texScrollScale;
    float topSpeed;
    float accel;
    float rotRate;
    float rotAccel;
    float maxBodyPitch;
    float maxBodyRoll;
    float fakeBodyAccelPitch;
    float fakeBodyAccelRoll;
    float fakeBodyVelPitch;
    float fakeBodyVelRoll;
    float fakeBodySideVelPitch;
    float fakeBodyPitchStrength;
    float fakeBodyRollStrength;
    float fakeBodyPitchDampening;
    float fakeBodyRollDampening;
    float fakeBodyBoatRockingAmplitude;
    float fakeBodyBoatRockingPeriod;
    float fakeBodyBoatRockingRotationPeriod;
    float fakeBodyBoatRockingFadeoutSpeed;
    float boatBouncingMinForce;
    float boatBouncingMaxForce;
    float boatBouncingRate;
    float boatBouncingFadeinSpeed;
    float boatBouncingFadeoutSteeringAngle;
    float collisionDamage;
    float collisionSpeed;
    float killcamOffset[3];
    int playerProtected;
    int bulletDamage;
    int armorPiercingDamage;
    int grenadeDamage;
    int projectileDamage;
    int projectileSplashDamage;
    int heavyExplosiveDamage;
    VehiclePhysDef vehPhysDef;
    float boostDuration;
    float boostRechargeTime;
    float boostAcceleration;
    float suspensionTravel;
    float maxSteeringAngle;
    float steeringLerp;
    float minSteeringScale;
    float minSteeringSpeed;
    int camLookEnabled;
    float camLerp;
    float camPitchInfluence;
    float camRollInfluence;
    float camFovIncrease;
    float camFovOffset;
    float camFovSpeed;
    const char* turretWeaponName;
    WeaponCompleteDef* turretWeapon;
    float turretHorizSpanLeft;
    float turretHorizSpanRight;
    float turretVertSpanUp;
    float turretVertSpanDown;
    float turretRotRate;
    snd_alias_list_t* turretSpinSnd;
    snd_alias_list_t* turretStopSnd;
    int trophyEnabled;
    float trophyRadius;
    float trophyInactiveRadius;
    int trophyAmmoCount;
    float trophyReloadTime;
    unsigned short trophyTags[4];
    Material* compassFriendlyIcon;
    Material* compassEnemyIcon;
    int compassIconWidth;
    int compassIconHeight;
    snd_alias_list_t* idleLowSnd;
    snd_alias_list_t* idleHighSnd;
    snd_alias_list_t* engineLowSnd;
    snd_alias_list_t* engineHighSnd;
    float engineSndSpeed;
    snd_alias_list_t* engineStartUpSnd;
    int engineStartUpLength;
    snd_alias_list_t* engineShutdownSnd;
    snd_alias_list_t* engineIdleSnd;
    snd_alias_list_t* engineSustainSnd;
    snd_alias_list_t* engineRampUpSnd;
    int engineRampUpLength;
    snd_alias_list_t* engineRampDownSnd;
    int engineRampDownLength;
    snd_alias_list_t* suspensionSoftSnd;
    float suspensionSoftCompression;
    snd_alias_list_t* suspensionHardSnd;
    float suspensionHardCompression;
    snd_alias_list_t* collisionSnd;
    float collisionBlendSpeed;
    snd_alias_list_t* speedSnd;
    float speedSndBlendSpeed;
    const char* surfaceSndPrefix;
    snd_alias_list_t* surfaceSnds[31];
    float surfaceSndBlendSpeed;
    float slideVolume;
    float slideBlendSpeed;
    float inAirPitch;
  };

  // 1300
  //
  struct AddonMapEnts
  {
    const char* name;
    char* entityString;
    int numEntityChars;
    MapTriggers trigger;
  };

  // 1301
  //
  union XAssetHeader
  {
    PhysPreset* physPreset;
    PhysCollmap* physCollmap;
    XAnimParts* parts;
    XModelSurfs* modelSurfs;
    XModel* model;
    Material* material;
    MaterialPixelShader* pixelShader;
    MaterialVertexShader* vertexShader;
    MaterialVertexDeclaration* vertexDecl;
    MaterialTechniqueSet* techniqueSet;
    GfxImage* image;
    snd_alias_list_t* sound;
    SndCurve* sndCurve;
    LoadedSound* loadSnd;
    clipMap_t* clipMap;
    ComWorld* comWorld;
    GameWorldSp* gameWorldSp;
    GameWorldMp* gameWorldMp;
    MapEnts* mapEnts;
    FxWorld* fxWorld;
    GfxWorld* gfxWorld;
    GfxLightDef* lightDef;
    Font_s* font;
    MenuList* menuList;
    menuDef_t* menu;
    LocalizeEntry* localize;
    WeaponCompleteDef* weapon;
    SndDriverGlobals* sndDriverGlobals;
    FxEffectDef* fx;
    FxImpactTable* impactFx;
    RawFile* rawfile;
    StringTable* stringTable;
    LeaderboardDef* leaderboardDef;
    StructuredDataDefSet* structuredDataDefSet;
    TracerDef* tracerDef;
    VehicleDef* vehDef;
    AddonMapEnts* addonMapEnts;
    void* data;
  };

  // 1303
  //
  struct cmd_function_s
  {
    cmd_function_s *next;
    const char *name;
    const char *autoCompleteDir;
    const char *autoCompleteExt;
    void (__cdecl *function)();
    int flags;
  };

  // 1315
  //
  struct netadr_t
  {
    netadrtype_t type;
    char ip[4];
    unsigned short port;
    char ipx[10];
  };

  // 1362
  //
  struct XNKID
  {
    char ab[8];
  };

  // 1363
  //
  struct XNKEY
  {
    char ab[16];
  };

  // 1364
  //
  struct XSESSION_INFO
  {
    XNKID sessionID;
    XNADDR hostAddress;
    XNKEY keyExchangeKey;
  };

  // 1474
  //
  struct msg_t
  {
    int overflowed;
    int readOnly;
    char* data;
    char* splitData;
    int maxsize;
    int cursize;
    int splitSize;
    int readcount;
    int bit;
    int lastEntityRef;
  };

  // 1591
  //
  struct UILocalVar
  {
    UILocalVarType type;
    const char* name;

    // 1590
    //
    union
    {
      int integer;
      float value;
      const char* string;
    } u;
  };

  // 1592
  //
  struct UILocalVarContext
  {
    UILocalVar table[256];
  };

  // 1593
  //
  struct UiContext
  {
    int localClientNum;
    float bias;
    int realTime;
    int frameTime;

    // 1589
    //
    struct
    {
      float x;
      float y;
      int lastMoveTime;
    } cursor;

    int isCursorVisible;
    int paintFull;
    int screenWidth;
    int screenHeight;
    float screenAspect;
    float FPS;
    float blurRadiusOut;
    menuDef_t* Menus[640];
    int menuCount;
    menuDef_t* menuStack[16];
    int openMenuCount;
    UILocalVarContext localVars;
  };

  // 1724
  //
  struct ScreenPlacement
  {
    vec2_t scaleVirtualToReal;
    vec2_t scaleVirtualToFull;
    vec2_t scaleRealToVirtual;
    vec2_t realViewportPosition;
    vec2_t realViewportSize;
    vec2_t virtualViewableMin;
    vec2_t virtualViewableMax;
    vec2_t realViewableMin;
    vec2_t realViewableMax;
    vec2_t virtualAdjustableMin;
    vec2_t virtualAdjustableMax;
    vec2_t realAdjustableMin;
    vec2_t realAdjustableMax;
    vec2_t subScreenLeft;
  };

  // 1842
  //
  struct TempPriority
  {
    void *threadHandle;
    int oldPriority;
  };

  // 1851
  //
  struct FastCriticalSection
  {
    volatile long readCount;
    volatile long writeCount;
    TempPriority tempPriority;
  };

  // 2096
  //
  struct ScriptStringList
  {
    int count;
    const char** strings;
  };

  // 2370
  struct field_t
  {
    int cursor;
    int scroll;
    int drawWidth;
    int widthInPixels;
    float charHeight;
    int fixedSize;
    char buffer[256];
  };

  // 2686
  //
  struct XAsset
  {
    XAssetType type;
    XAssetHeader header;
  };

  // 2687
  //
  struct XAssetList
  {
    ScriptStringList stringList;
    int assetCount;
    XAsset* assets;
  };

  // 2795
  //
  struct XAssetEntry
  {
    XAsset asset;
    char zoneIndex;
    volatile char inuseMask;
    bool printedMissingAsset;
    unsigned short nextHash;
    unsigned short nextOverride;
  };

  // 2796
  //
  union XAssetEntryPoolEntry
  {
    XAssetEntry entry;
    XAssetEntryPoolEntry* next;
  };

  // 2871
  //
  struct keyname_t
  {
    const char* name;
    int keynum;
  };

  // 2872
  //
  struct KeyState
  {
    int down;
    int repeats;
    const char* binding;
  };

  // 2873
  //
  struct PlayerKeyState
  {
    field_t chatField;
    int chat_team;
    int overstrikeMode;
    int anyKeyDown;
    KeyState keys[256];
    LocSelInputState locSelInputState;
  };

  using  bdAlloc_t = void* (*) (size_t);
  inline bdAlloc_t bdAlloc = reinterpret_cast<bdAlloc_t> (0x140315420);

  using  bdBandwidthTestClientInit_t = bool (*) (void*);
  inline bdBandwidthTestClientInit_t bdBandwidthTestClientInit = reinterpret_cast<bdBandwidthTestClientInit_t> (0x140321560);

  using  bdBandwidthTestClientStart_t = void (*) (void*, int);
  inline bdBandwidthTestClientStart_t bdBandwidthTestClientStart = reinterpret_cast<bdBandwidthTestClientStart_t> (0x140322260);

  using  bdLobbyConnectionStartTask_t = int32_t (*) (void*, void**, uint8_t, uint8_t, void*, float);
  inline bdLobbyConnectionStartTask_t bdLobbyConnectionStartTask = reinterpret_cast<bdLobbyConnectionStartTask_t> (0x140322CC0);

  using  bdLobbyService_t = void* (*) ();
  inline bdLobbyService_t bdLobbyService = reinterpret_cast<bdLobbyService_t> (0x140136210);

  using  bdLobbyServiceImplConnect_t = bool (*) (void*, void*, void*, bool);
  inline bdLobbyServiceImplConnect_t bdLobbyServiceImplConnect = reinterpret_cast<bdLobbyServiceImplConnect_t> (0x14031c750);

  using  bdLobbyServiceImplDisconnect_t = void (*) (void*, int);
  inline bdLobbyServiceImplDisconnect_t bdLobbyServiceImplDisconnect = reinterpret_cast<bdLobbyServiceImplDisconnect_t> (0x14031c950);

  using  bdLobbyServiceImplGetMatchmaking_t = void* (*) (void*);
  inline bdLobbyServiceImplGetMatchmaking_t bdLobbyServiceImplGetMatchmaking = reinterpret_cast<bdLobbyServiceImplGetMatchmaking_t> (0x14031c990);

  using  bdLobbyServiceImplGetPerformance_t = void* (*) (void*);
  inline bdLobbyServiceImplGetPerformance_t bdLobbyServiceImplGetPerformance = reinterpret_cast<bdLobbyServiceImplGetPerformance_t> (0x14031ca70);

  using  bdLobbyServiceImplGetStatus_t = int32_t (*) (void*);
  inline bdLobbyServiceImplGetStatus_t bdLobbyServiceImplGetStatus = reinterpret_cast<bdLobbyServiceImplGetStatus_t> (0x14031cb10);

  using  bdLobbyServiceImplGetStorage_t = void* (*) (void*);
  inline bdLobbyServiceImplGetStorage_t bdLobbyServiceImplGetStorage = reinterpret_cast<bdLobbyServiceImplGetStorage_t> (0x14031cff0);

  using  bdLobbyServiceImplGetTaskMgr_t = void* (*) (void*);
  inline bdLobbyServiceImplGetTaskMgr_t bdLobbyServiceImplGetTaskMgr = reinterpret_cast<bdLobbyServiceImplGetTaskMgr_t> (0x14031CA30);

  using  bdLogMessage_t = void (*) (int, const char*, const char*, const char*, const char*, int, const char*, ...);
  inline bdLogMessage_t bdLogMessage = reinterpret_cast<bdLogMessage_t> (0x140331F60);

  using  Cbuf_AddText_t = void (*) (int, const char *);
  inline Cbuf_AddText_t Cbuf_AddText = reinterpret_cast<Cbuf_AddText_t> (0x1401EC4A0);

  using  Cbuf_ExecuteBuffer_t = void (*) (unsigned int, unsigned int, uint8_t *);
  inline Cbuf_ExecuteBuffer_t Cbuf_ExecuteBuffer = reinterpret_cast<Cbuf_ExecuteBuffer_t> (0x1401EC780);

  using  CG_DrawFullScreenUI_t = void (*) (int localClientNum);
  inline CG_DrawFullScreenUI_t CG_DrawFullScreenUI = reinterpret_cast<CG_DrawFullScreenUI_t> (0x1400CDDB0);

  using  CL_CharEvent_t = void (*) (int localClientNum, int key);
  inline CL_CharEvent_t CL_CharEvent = reinterpret_cast<CL_CharEvent_t> (0x1400EA8A0);

  using  CL_ConnectFromParty_t = void (*) (int controller_index, XSESSION_INFO* session, netadr_t address, int, int, const char* mapname, const char* gametype);
  inline CL_ConnectFromParty_t CL_ConnectFromParty = reinterpret_cast<CL_ConnectFromParty_t> (0x1400F5220);

  using  CL_ConsolePrint_t = void (*) (int localClientNum, int channel, const char *txt, int duration, int pixelWidth, int flags);
  inline CL_ConsolePrint_t CL_ConsolePrint = reinterpret_cast<CL_ConsolePrint_t> (0x1400E7AD0);

  using  CL_ConsolePrint_AddLine_t = __int64 (*) (int localClientNum, int channel, const char *txt, int duration, int pixelWidth, char color, int flags);
  inline CL_ConsolePrint_AddLine_t CL_ConsolePrint_AddLine = reinterpret_cast<CL_ConsolePrint_AddLine_t> (0x1400E7B50);

  using  CL_DispatchConnectionlessPacket_t = void (*) (int local_client_num, netadr_t* from, msg_t* msg);
  inline CL_DispatchConnectionlessPacket_t CL_DispatchConnectionlessPacket = reinterpret_cast<CL_DispatchConnectionlessPacket_t> (0x1400F5FE0);

  using  CL_LocalClientNumFromControllerIndex_t = __int64 (*) (void);
  inline CL_LocalClientNumFromControllerIndex_t CL_LocalClientNumFromControllerIndex = reinterpret_cast<CL_LocalClientNumFromControllerIndex_t> (0x1400EF8F0);

  using  CL_KeyEvent_t = void (*) (int localClientNum, int key, int down, unsigned int time);
  inline CL_KeyEvent_t CL_KeyEvent = reinterpret_cast<CL_KeyEvent_t> (0x1400EAA80);

  using  CL_SelectStringTableEntryInDvar_f_t = void (__fastcall *) (void);
  inline CL_SelectStringTableEntryInDvar_f_t CL_SelectStringTableEntryInDvar_f = reinterpret_cast<CL_SelectStringTableEntryInDvar_f_t> (0x14026DD30);

  using  ClientConnect_t = void * (*) (int, __int16);
  inline ClientConnect_t ClientConnect = reinterpret_cast<ClientConnect_t> (0x14019A470);

  using  Cmd_AddCommandInternal_t = void (*) (const char *cmdName, void (__fastcall *function)(), cmd_function_s *allocedCmd);
  inline Cmd_AddCommandInternal_t Cmd_AddCommandInternal = reinterpret_cast<Cmd_AddCommandInternal_t> (0x1401EC990);

  using  Com_Error_t = void (*) (int, const char *, ...);
  inline Com_Error_t Com_Error = reinterpret_cast<Com_Error_t> (0x1401F8FD0);

  using  Com_ErrorEntered_t = bool (*) (void);
  inline Com_ErrorEntered_t Com_ErrorEntered = reinterpret_cast<Com_ErrorEntered_t> (0x1401F9620);

  using  Com_Quit_f_t = void (*) (void);
  inline Com_Quit_f_t Com_Quit_f = reinterpret_cast<Com_Quit_f_t> (0x1401FB240);

  using  Con_Init_t = void (*) (void);
  inline Con_Init_t Con_Init = reinterpret_cast<Con_Init_t> (0x1400E9360);

  using  Con_OneTimeInit_t = void (*) ();
  inline Con_OneTimeInit_t Con_OneTimeInit = reinterpret_cast<Con_OneTimeInit_t> (0x1400E9960);

  using  Com_Frame_Try_Block_Function_t = int64_t (*) ();
  inline Com_Frame_Try_Block_Function_t Com_Frame_Try_Block_Function = reinterpret_cast<Com_Frame_Try_Block_Function_t> (0x1401F9930);

  using  CopyStringInternal_t = const char* (*) (const char *);
  inline CopyStringInternal_t CopyStringInternal = reinterpret_cast<CopyStringInternal_t> (0x140281470);

  using  DB_FindXAssetHeader_t = XAssetHeader (*) (XAssetType, const char* name);
  inline DB_FindXAssetHeader_t DB_FindXAssetHeader = reinterpret_cast<DB_FindXAssetHeader_t> (0x140129220);

  using  Dvar_AddCommands_t = void (*) (void);
  inline Dvar_AddCommands_t Dvar_AddCommands = reinterpret_cast<Dvar_AddCommands_t> (0x140200EE0);

  using  Dvar_AddFlags_t = void (*) (dvar_t *dvar, DvarFlags flags);
  inline Dvar_AddFlags_t Dvar_AddFlags = reinterpret_cast<Dvar_AddFlags_t> (0x140286A80);

  using  Dvar_AssignCurrentStringValue_t = void (*) (dvar_t *dvar, DvarValue *dest, const char *string);
  inline Dvar_AssignCurrentStringValue_t Dvar_AssignCurrentStringValue = reinterpret_cast<Dvar_AssignCurrentStringValue_t> (0x140286B60);

  using  Dvar_AssignResetStringValue_t = void (*) (dvar_t *dvar, DvarValue *dest, const char *string);
  inline Dvar_AssignResetStringValue_t Dvar_AssignResetStringValue = reinterpret_cast<Dvar_AssignResetStringValue_t> (0x140286C10);

  using  Dvar_ClearModified_t = void (*) (dvar_t *dvar);
  inline Dvar_ClearModified_t Dvar_ClearModified = reinterpret_cast<Dvar_ClearModified_t> (0x140286FA0);

  using  Dvar_Command_t = int (*) (void);
  inline Dvar_Command_t Dvar_Command = reinterpret_cast<Dvar_Command_t> (0x140200F90);

  using  Dvar_DisplayableValue_t = const char * (*) (dvar_t *dvar);
  inline Dvar_DisplayableValue_t Dvar_DisplayableValue = reinterpret_cast<Dvar_DisplayableValue_t> (0x140286FF0);

  using  Dvar_FindMalleableVar_t = dvar_t * (*) (const char *dvarName);
  inline Dvar_FindMalleableVar_t Dvar_FindMalleableVar = reinterpret_cast<Dvar_FindMalleableVar_t> (0x140287080);

  using  Dvar_FindVar_t = dvar_t* (*) (const char* name);
  inline Dvar_FindVar_t Dvar_FindVar = reinterpret_cast<Dvar_FindVar_t> (0x140287170);

  using  Dvar_ForEach_t = void (*) (void (__fastcall *callback)(const dvar_t *, void*), void* userData);
  inline Dvar_ForEach_t Dvar_ForEach = reinterpret_cast<Dvar_ForEach_t> (0x140287180);

  using  Dvar_GetBool_t = bool (*) (const char *dvarName);
  inline Dvar_GetBool_t Dvar_GetBool = reinterpret_cast<Dvar_GetBool_t> (0x140287220);

  using  Dvar_GetCombinedString_t = void (*) (char *out, int startArg);
  inline Dvar_GetCombinedString_t Dvar_GetCombinedString = reinterpret_cast<Dvar_GetCombinedString_t> (0x140201060);

  using  Dvar_GetFloat_t = float (*) (const char *dvarName);
  inline Dvar_GetFloat_t Dvar_GetFloat = reinterpret_cast<Dvar_GetFloat_t> (0x140287260);

  using  Dvar_GetIntByName_t = int (*) (const char *dvarName);
  inline Dvar_GetIntByName_t Dvar_GetIntByName = reinterpret_cast<Dvar_GetIntByName_t> (0x1402872A0);

  using  Dvar_GetString_t = const char * (*) (const char *dvarName);
  inline Dvar_GetString_t Dvar_GetString = reinterpret_cast<Dvar_GetString_t> (0x1402872E0);

  using  Dvar_GetVariantString_t = const char * (*) (const char *dvarName);
  inline Dvar_GetVariantString_t Dvar_GetVariantString = reinterpret_cast<Dvar_GetVariantString_t> (0x140287400);

  using  Dvar_Init_t = void (*) (void);
  inline Dvar_Init_t Dvar_Init = reinterpret_cast<Dvar_Init_t> (0x140287520);

  using  Dvar_IsValidName_t = bool (*) (const char *dvarName);
  inline Dvar_IsValidName_t Dvar_IsValidName = reinterpret_cast<Dvar_IsValidName_t> (0x140287550);

  using  Dvar_LoadDvarsAddFlags_t = void (*) (void *memFile, DvarFlags flags);
  inline Dvar_LoadDvarsAddFlags_t Dvar_LoadDvarsAddFlags = reinterpret_cast<Dvar_LoadDvarsAddFlags_t> (0x1402875C0);

  using  Dvar_RegisterBool_t = dvar_t* (*) (const char* name, bool value, unsigned int flags, const char* description);
  inline Dvar_RegisterBool_t Dvar_RegisterBool = reinterpret_cast<Dvar_RegisterBool_t> (0x140287CE0);

  using  Dvar_RegisterColor_t = dvar_t * (*) (const char *dvarName, float r, float g, float b, float a, DvarFlags flags, const char *description);
  inline Dvar_RegisterColor_t Dvar_RegisterColor = reinterpret_cast<Dvar_RegisterColor_t> (0x140287DC0);

  using  Dvar_RegisterEnum_t = dvar_t * (*) (const char *dvarName, const char **valueList, int defaultIndex, DvarFlags flags, const char *description);
  inline Dvar_RegisterEnum_t Dvar_RegisterEnum = reinterpret_cast<Dvar_RegisterEnum_t> (0x140287FC0);

  using  Dvar_RegisterFloat_t = dvar_t * (*) (const char *dvarName, float value, float min, float max, DvarFlags flags, const char *description);
  inline Dvar_RegisterFloat_t Dvar_RegisterFloat = reinterpret_cast<Dvar_RegisterFloat_t> (0x1402880C0);

  using  Dvar_RegisterInt_t = dvar_t * (*) (const char *dvarName, int value, int min, int max, DvarFlags flags, const char *description);
  inline Dvar_RegisterInt_t Dvar_RegisterInt = reinterpret_cast<Dvar_RegisterInt_t> (0x1402881F0);

  using  Dvar_RegisterString_t = dvar_t * (*) (const char *dvarName, const char *value, DvarFlags flags, const char *description);
  inline Dvar_RegisterString_t Dvar_RegisterString = reinterpret_cast<Dvar_RegisterString_t> (0x140288590);

  using  Dvar_RegisterVariant_t = dvar_t * (*) (const char *dvarName, dvarType type, DvarFlags flags, const DvarValue *value, const DvarLimits *domain, const char *description);
  inline Dvar_RegisterVariant_t Dvar_RegisterVariant = reinterpret_cast<Dvar_RegisterVariant_t> (0x1402882E0);

  using  Dvar_RegisterVec2_t = dvar_t * (*) (const char *dvarName, float x, float y, float min, float max, DvarFlags flags, const char *description);
  inline Dvar_RegisterVec2_t Dvar_RegisterVec2 = reinterpret_cast<Dvar_RegisterVec2_t> (0x140288660);

  using  Dvar_RegisterVec3_t = dvar_t * (*) (const char *dvarName, float x, float y, float z, float min, float max, DvarFlags flags, const char *description);
  inline Dvar_RegisterVec3_t Dvar_RegisterVec3 = reinterpret_cast<Dvar_RegisterVec3_t> (0x140288780);

  using  Dvar_RegisterVec3Color_t = dvar_t * (*) (const char *dvarName, float r, float g, float b, DvarFlags flags, const char *description);
  inline Dvar_RegisterVec3Color_t Dvar_RegisterVec3Color = reinterpret_cast<Dvar_RegisterVec3Color_t> (0x1402888B0);

  using  Dvar_RegisterVec4_t = dvar_t * (*) (const char *dvarName, float x, float y, float z, float w, float min, float max, DvarFlags flags, const char *description);
  inline Dvar_RegisterVec4_t Dvar_RegisterVec4 = reinterpret_cast<Dvar_RegisterVec4_t> (0x1402889D0);

  using  Dvar_Reset_f_t = void (__fastcall *) (void);
  inline Dvar_Reset_f_t Dvar_Reset_f = reinterpret_cast<Dvar_Reset_f_t> (0x140201350);

  using  Dvar_Reset_t = void (*) (dvar_t *dvar, DvarSetSource setSource);
  inline Dvar_Reset_t Dvar_Reset = reinterpret_cast<Dvar_Reset_t> (0x140288DB0);

  using  Dvar_ResetScriptInfo_t = void (*) (void);
  inline Dvar_ResetScriptInfo_t Dvar_ResetScriptInfo = reinterpret_cast<Dvar_ResetScriptInfo_t> (0x140288DD0);

  using  Dvar_Set_f_t = void (__fastcall *) (void);
  inline Dvar_Set_f_t Dvar_Set_f = reinterpret_cast<Dvar_Set_f_t> (0x140201590);

  using  Dvar_SetA_f_t = void (__fastcall *) (void);
  inline Dvar_SetA_f_t Dvar_SetA_f = reinterpret_cast<Dvar_SetA_f_t> (0x1402013A0);

  using  Dvar_SetBool_t = void (*) (dvar_t *dvar, bool value);
  inline Dvar_SetBool_t Dvar_SetBool = reinterpret_cast<Dvar_SetBool_t> (0x140288FB0);

  using  Dvar_SetBoolByName_t = void (*) (const char *nname, bool value);
  inline Dvar_SetBoolByName_t Dvar_SetBoolByName = reinterpret_cast<Dvar_SetBoolByName_t> (0x140289000);

  using  Dvar_SetCommand_t = void (*) (const char *dvarName, const char *string);
  inline Dvar_SetCommand_t Dvar_SetCommand = reinterpret_cast<Dvar_SetCommand_t> (0x1402892A0);

  using  Dvar_SetDomainFunc_t = void (*) (dvar_t *, bool (*callback)(dvar_t*, DvarValue *));
  inline Dvar_SetDomainFunc_t Dvar_SetDomainFunc = reinterpret_cast<Dvar_SetDomainFunc_t> (0x140289350);

  using  Dvar_SetFloat_t = void (*) (dvar_t *dvar, float value);
  inline Dvar_SetFloat_t Dvar_SetFloat = reinterpret_cast<Dvar_SetFloat_t> (0x1402893A0);

  using  Dvar_SetFromDvar_f_t = void (__fastcall *) (void);
  inline Dvar_SetFromDvar_f_t Dvar_SetFromDvar_f = reinterpret_cast<Dvar_SetFromDvar_f_t> (0x140201510);

  using  Dvar_SetFromStringByName_t = dvar_t * (*) (const char*, const char*);
  inline Dvar_SetFromStringByName_t Dvar_SetFromStringByName = reinterpret_cast<Dvar_SetFromStringByName_t> (0x140289570);

  using  Dvar_SetFromStringFromSource_t = void (*) (dvar_t *dvar, const char *string, DvarSetSource source);
  inline Dvar_SetFromStringFromSource_t Dvar_SetFromStringFromSource = reinterpret_cast<Dvar_SetFromStringFromSource_t> (0x140289640);

  using  Dvar_SetInt_t = void (*) (dvar_t *dvar, int value);
  inline Dvar_SetInt_t Dvar_SetInt = reinterpret_cast<Dvar_SetInt_t> (0x1402896E0);

  using  Dvar_SetIntByName_t = void (*) (const char *name, int value);
  inline Dvar_SetIntByName_t Dvar_SetIntByName = reinterpret_cast<Dvar_SetIntByName_t> (0x140289740);

  using  Dvar_SetLatchedValue_t = void (*) (dvar_t *dvar, DvarValue *value);
  inline Dvar_SetLatchedValue_t Dvar_SetLatchedValue = reinterpret_cast<Dvar_SetLatchedValue_t> (0x140289910);

  using  Dvar_SetModified_t = void (*) (dvar_t *dvar);
  inline Dvar_SetModified_t Dvar_SetModified = reinterpret_cast<Dvar_SetModified_t> (0x140289A70);

  using  Dvar_SetString_t = void (*) (dvar_t *dvar, const char *value);
  inline Dvar_SetString_t Dvar_SetString = reinterpret_cast<Dvar_SetString_t> (0x140289A80);

  using  Dvar_SetStringByName_t = void (*) (const char *name, const char *value);
  inline Dvar_SetStringByName_t Dvar_SetStringByName = reinterpret_cast<Dvar_SetStringByName_t> (0x140289AE0);

  using  Dvar_SetVariant_t = void (*) (dvar_t *dvar, DvarValue value, DvarSetSource source);
  inline Dvar_SetVariant_t Dvar_SetVariant = reinterpret_cast<Dvar_SetVariant_t> (0x140289B80);

  using  Dvar_Sort_t = __int64 (*) (void);
  inline Dvar_Sort_t Dvar_Sort = reinterpret_cast<Dvar_Sort_t> (0x14028A020);

  using  Dvar_StringToColor_t = void (*) (const char *string, DvarValue *domain);
  inline Dvar_StringToColor_t Dvar_StringToColor = reinterpret_cast<Dvar_StringToColor_t> (0x14028A090);

  using  Dvar_StringToEnum_t = int (*) (const DvarLimits domain, const char *string);
  inline Dvar_StringToEnum_t Dvar_StringToEnum = reinterpret_cast<Dvar_StringToEnum_t> (0x14028A1C0);

  using  Dvar_StringToValue_t = DvarValue * (*) (DvarValue *dvarValue, dvarType type, DvarLimits *domain, const char *string);
  inline Dvar_StringToValue_t Dvar_StringToValue = reinterpret_cast<Dvar_StringToValue_t> (0x14028A2C0);

  using  Dvar_ToggleInternal_t = bool (__fastcall *) (void);
  inline Dvar_ToggleInternal_t Dvar_ToggleInternal = reinterpret_cast<Dvar_ToggleInternal_t> (0x140201620);

  using  Dvar_TogglePrint_f_t = void (__fastcall *) (void);
  inline Dvar_TogglePrint_f_t Dvar_TogglePrint_f = reinterpret_cast<Dvar_TogglePrint_f_t> (0x140201940);

  using  Dvar_UpdateResetValue_t = void (*) (dvar_t *dvar, DvarValue value);
  inline Dvar_UpdateResetValue_t Dvar_UpdateResetValue = reinterpret_cast<Dvar_UpdateResetValue_t> (0x14028A450);

  using  Dvar_ValueInDomain_t = bool (*) (dvarType type, DvarValue value, DvarLimits domain);
  inline Dvar_ValueInDomain_t Dvar_ValueInDomain = reinterpret_cast<Dvar_ValueInDomain_t> (0x14028A550);

  using  Dvar_ValuesEqual_t = bool (*) (dvarType type, DvarValue val0, DvarValue val1);
  inline Dvar_ValuesEqual_t Dvar_ValuesEqual = reinterpret_cast<Dvar_ValuesEqual_t> (0x14028A860);

  using  Dvar_ValueToString_t = const char * (*) (dvar_t *dvar, DvarValue value);
  inline Dvar_ValueToString_t Dvar_ValueToString = reinterpret_cast<Dvar_ValueToString_t> (0x14028A690);

  using  DW_SendPush_t = void (*) ();
  inline DW_SendPush_t DW_SendPush = reinterpret_cast<DW_SendPush_t> (0x14012ED70);

  using  FreeStringInternal_t = void (*) (const char*);
  inline FreeStringInternal_t FreeStringInternal = reinterpret_cast<FreeStringInternal_t> (0x140281490);

  using  I_strcmp_t = __int64 (*) (const char *s0, const char *s1);
  inline I_strcmp_t I_strcmp = reinterpret_cast<I_strcmp_t> (0x14028E200);

  using  I_stricmp_t = int (*) (const char* s0, const char* s1);
  inline I_stricmp_t I_stricmp = reinterpret_cast<I_stricmp_t> (0x14028E250);

  using  I_strlwr_t = char * (*) (char *s);
  inline I_strlwr_t I_strlwr = reinterpret_cast<I_strlwr_t> (0x14028E400);

  using  I_strncat_t = void (*) (char *dest, int size, const char *src);
  inline I_strncat_t I_strncat = reinterpret_cast<I_strncat_t> (0x14028E430);

  using  I_strncmp_t = __int64 (*) (const char *s0, const char *s1, int n);
  inline I_strncmp_t I_strncmp = reinterpret_cast<I_strncmp_t> (0x14028E4B0);

  using  I_strncpyz_t = void (*) (char *dest, const char *src, int destsize);
  inline I_strncpyz_t I_strncpyz = reinterpret_cast<I_strncpyz_t> (0x14028E500);

  using  I_strnicmp_t = int (*) (const char *s0, const char *s1, int n);
  inline I_strnicmp_t I_strnicmp = reinterpret_cast<I_strnicmp_t> (0x14028E530);

  using  IWNet_Frame_t = void (*) (int);
  inline IWNet_Frame_t IWNet_Frame = reinterpret_cast<IWNet_Frame_t> (0x14012DF70);

  using  Live_Frame_t = void (*) (unsigned int);
  inline Live_Frame_t Live_Frame = reinterpret_cast<Live_Frame_t> (0x1402A6040);

  using  Live_SetPlaylistVersion_t = int64_t (*) (void);
  inline Live_SetPlaylistVersion_t Live_SetPlaylistVersion = reinterpret_cast<Live_SetPlaylistVersion_t> (0x1402A7300);

  using  Live_StartPrivateParty_t = void (*) (int controller_index);
  inline Live_StartPrivateParty_t Live_StartPrivateParty = reinterpret_cast<Live_StartPrivateParty_t> (0x1400FABC0);

  using  Live_ThrowError_t = void (*) (int, const char *);
  inline Live_ThrowError_t Live_ThrowError = reinterpret_cast<Live_ThrowError_t> (0x1402A7600);

  using  LiveStorage_DownloadStatsFromDir_t = void (*) (int);
  inline LiveStorage_DownloadStatsFromDir_t LiveStorage_DownloadStatsFromDir = reinterpret_cast<LiveStorage_DownloadStatsFromDir_t> (0x140133F00);

  using  LiveStorage_FetchPlaylists_t = void (*) (int controller_index);
  inline LiveStorage_FetchPlaylists_t LiveStorage_FetchPlaylists = reinterpret_cast<LiveStorage_FetchPlaylists_t> (0x1402AA840);

  using  LiveStorage_IsWaitingOnPlaylists_t = bool (*) ();
  inline LiveStorage_IsWaitingOnPlaylists_t LiveStorage_IsWaitingOnPlaylists = reinterpret_cast<LiveStorage_IsWaitingOnPlaylists_t> (0x1402AA9B0);

  using  LiveStorage_IsWaitingOnStats_t = char (*) (__int64);
  inline LiveStorage_IsWaitingOnStats_t LiveStorage_IsWaitingOnStats = reinterpret_cast<LiveStorage_IsWaitingOnStats_t> (0x1401FE130);

  using  NET_OutOfBandPrint_t = void (*) (int sock, netadr_t* address, const char* format, ...);
  inline NET_OutOfBandPrint_t NET_OutOfBandPrint = reinterpret_cast<NET_OutOfBandPrint_t> (0x140209FC0);

  using  NET_SendPacket_t = bool (*) (int sock, int length, const char* data, netadr_t address);
  inline NET_SendPacket_t NET_SendPacket = reinterpret_cast<NET_SendPacket_t> (0x14020A040);

  using  Party_SetState_t = void (*) (void* party, int state);
  inline Party_SetState_t Party_SetState = reinterpret_cast<Party_SetState_t> (0x140105570);

  using  Playlist_ParsePlaylists_t = int64_t (*) (uint8_t*);
  inline Playlist_ParsePlaylists_t Playlist_ParsePlaylists = reinterpret_cast<Playlist_ParsePlaylists_t> (0x14025D460);

  using  Playlist_ValidatePlaylistNum_t = void (*) (void);
  inline Playlist_ValidatePlaylistNum_t Playlist_ValidatePlaylistNum = reinterpret_cast<Playlist_ValidatePlaylistNum_t> (0x14025E250);

  using  R_AddCmdDrawStretchPic_t = void (*) (float x, float y, float width, float height, float s0, float t0, float s1, float t1, const float* color, Material* material);
  inline R_AddCmdDrawStretchPic_t R_AddCmdDrawStretchPic = reinterpret_cast<R_AddCmdDrawStretchPic_t> (0x14001ACE0);

  using  R_AddCmdDrawText_t = void (*) (const char *text, int maxChars, const Font_s *font, float x, float y, float xScale, float yScale, float rotation, const float *color, int style);
  inline R_AddCmdDrawText_t R_AddCmdDrawText = reinterpret_cast<R_AddCmdDrawText_t> (0x14001B260);

  using  R_LoadGraphicsAssets_t = void (*) (void);
  inline R_LoadGraphicsAssets_t R_LoadGraphicsAssets = reinterpret_cast<R_LoadGraphicsAssets_t> (0x14002FA70);

  using  R_TextWidth_t = int (*) (const char *text, int maxChars, Font_s *font);
  inline R_TextWidth_t R_TextWidth = reinterpret_cast<R_TextWidth_t> (0x140019870);

  using  R_TextHeight_t = int (*) (Font_s *font);
  inline R_TextHeight_t R_TextHeight = reinterpret_cast<R_TextHeight_t> (0x1400199C0);

  using  StringTable_GetAsset_t = __int64 (*) (const char *, __int64 *);
  inline StringTable_GetAsset_t StringTable_GetAsset = reinterpret_cast<StringTable_GetAsset_t> (0x140282E50);

  using  StringTable_GetColumnValueForRow_t = const CHAR * (*) (__int64, int, int);
  inline StringTable_GetColumnValueForRow_t StringTable_GetColumnValueForRow = reinterpret_cast<StringTable_GetColumnValueForRow_t> (0x140282E70);

  using  SV_ConnectionlessPacket_t = void (*) (netadr_t* from, msg_t* msg);
  inline SV_ConnectionlessPacket_t SV_ConnectionlessPacket = reinterpret_cast<SV_ConnectionlessPacket_t> (0x140240090);

  using  SV_GameSendServerCommand_t = void (*) (int client_num, int cmd_type, const char* text);
  inline SV_GameSendServerCommand_t SV_GameSendServerCommand = reinterpret_cast<SV_GameSendServerCommand_t> (0x1402369B0);

  using  Sys_Milliseconds_t = __int64 (*) (void);
  inline Sys_Milliseconds_t Sys_Milliseconds = reinterpret_cast<Sys_Milliseconds_t> (0x1402AA460);

  using  Sys_Print_t = void (*) (const char *msg);
  inline Sys_Print_t Sys_Print = reinterpret_cast<Sys_Print_t> (0x1402A8BB0);

  using  Sys_SendPacket_t = bool (*) (size_t len, const char *buf, const netadr_t *a3);
  inline Sys_SendPacket_t Sys_SendPacket = reinterpret_cast<Sys_SendPacket_t> (0x1402AA1B0);

  using  Sys_IsMainThread_t = bool (*) (void);
  inline Sys_IsMainThread_t Sys_IsMainThread = reinterpret_cast<Sys_IsMainThread_t> (0x14020DE70);

  using  ScrPlace_GetActivePlacement_t = ScreenPlacement * (*) (int localClientNum);
  inline ScrPlace_GetActivePlacement_t ScrPlace_GetActivePlacement = reinterpret_cast<ScrPlace_GetActivePlacement_t> (0x1400EF370);

  using  UI_AddMenuList_t = void (*) (UiContext*, MenuList*, int);
  inline UI_AddMenuList_t UI_AddMenuList = reinterpret_cast<UI_AddMenuList_t> (0x14026B6C0);

  using  UI_GetClientDC_t = UiContext* (*) ();
  inline UI_GetClientDC_t UI_GetClientDC = reinterpret_cast<UI_GetClientDC_t> (0x140271380);

  using  UI_GetFrontendContext_t = UiContext* (*) ();
  inline UI_GetFrontendContext_t UI_GetFrontendContext = reinterpret_cast<UI_GetFrontendContext_t> (0x140270A60);

  using  UI_LoadMenus_t = MenuList* (*) (const char* name);
  inline UI_LoadMenus_t UI_LoadMenus = reinterpret_cast<UI_LoadMenus_t> (0x14026C830);

  using  Uk_OnConnected_t = void (*) (int);
  inline Uk_OnConnected_t Uk_OnConnected = reinterpret_cast<Uk_OnConnected_t> (0x1402A6850);

  // Internal global variables.
  //
  inline volatile FastCriticalSection* g_dvarCritSect = reinterpret_cast<FastCriticalSection*> (0x14673D280);
  inline bool*                         areDvarsSorted = reinterpret_cast<bool*> (0x14673D270);
  inline int*                          dvarCount = reinterpret_cast<int*> (0x1466D3268);
  inline int**                         sv_dvar_modifiedFlags = reinterpret_cast<int**> (0x1466D3260);
  inline dvar_t*                       dvarPool = reinterpret_cast<dvar_t*> (0x1466DB270);
  inline dvar_t**                      sortedDvars = reinterpret_cast<dvar_t**> (0x1466D3270);
  inline dvar_t**                      dvarHashTable = reinterpret_cast<dvar_t**> (0x14673B270);
  inline CmdArgs*                      cmd_args = reinterpret_cast<CmdArgs*> (0x141C17810);
  inline PlayerKeyState*               playerKeys = reinterpret_cast<PlayerKeyState*> (0x14070DA30);
  inline clientUIActive_t*             clientUIActives (reinterpret_cast<clientUIActive_t*> (0x140718FB0));
  inline clientStatic_t*               cls = reinterpret_cast<clientStatic_t*> (0x140CA7BB0);

  inline constexpr int KEYCATCH_CONSOLE (1);
  inline constexpr int KEYCATCH_CGAME   (8);
  inline constexpr int KEYCATCH_UI      (16);
  inline constexpr int KEYCATCH_MESSAGE (32);
}
