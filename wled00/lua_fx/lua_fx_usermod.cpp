#include "lua_fx_usermod.h"

#include <cstdlib>

extern "C"
{
#include "../dependencies/lua/lua.h"
#include "../dependencies/lua/lauxlib.h"
#include "../dependencies/lua/lualib.h"
}

#define LuaCheck(x)                                  \
  do                                                 \
  {                                                  \
    const int result = x;                            \
    if (result != 0)                                 \
    {                                                \
      const char *str = lua_tostring(mLuaState, -1); \
      sDebugPrint("Lua error: %s", str);                  \
    }                                                \
  } while (0)

const char *exampleModule = R"(
    LuaPlugin = {}
    local tickCount = 0
    
    function LuaPlugin.loop()
      tickCount = tickCount + 1
      local color = (255 << 24) | (tickCount&255)
      set_segment_color(0, 0, 1, color)
    end

    function LuaPlugin.setup()
      print('LuaPlugin.setup')
    end

    function LuaPlugin.connected()
      print('LuaPlugin.connected')
    end
    
    return LuaPlugin
)";

static void sDebugPrint(const char *format, ...)
{
  va_list arglist;
  va_start(arglist, format);
  //vprintf(format, arglist);
  va_end(arglist);
}

LuaFXUsermod::LuaFXUsermod()
    : mLuaState(nullptr)
    , mLastTime(0)
{
  sDebugPrint("LuaFXUsermod::LuaFXUsermod()\n");
}

void LuaFXUsermod::loop()
{
  _callOnAllPlugins("loop");

  // Garbage collect for fun
  lua_gc(mLuaState, LUA_GCSTEP, 1);
}

void LuaFXUsermod::setup()
{
  sDebugPrint("LuaFXUsermod::setup1()\n");
  char buff[256];
  int error;
  mLuaState = luaL_newstate();

  // Only bind the modules we want, we have very little ram
  luaopen_base(mLuaState);
  luaopen_math(mLuaState);

  _bindFunctions();

  LuaCheck(luaL_dostring(mLuaState, exampleModule));
  if (lua_istable(mLuaState, -1) == 0)
  {
    // TODO: Not a table
  }
  else
  {
    mPluginRefs.emplace_back(luaL_ref(mLuaState, LUA_REGISTRYINDEX));
  }

  _dumpLuaStack();

  _callOnAllPlugins("setup");

  //lua_close(mLuaState);
}
void LuaFXUsermod::connected()
{
  _callOnAllPlugins("connected");
}
void LuaFXUsermod::addToJsonState(JsonObject &obj)
{
}
void LuaFXUsermod::addToJsonInfo(JsonObject &obj)
{
}
void LuaFXUsermod::readFromJsonState(JsonObject &obj)
{
}

void LuaFXUsermod::_callOnAllPlugins(const char *method)
{
  sDebugPrint("LuaFXUsermod::_callOnAllPlugins - Calling method '%s'\n", method);
  
  for (auto &&pluginRef : mPluginRefs)
  {
    //sDebugPrint("Before %d\n", pluginRef);
    //_dumpLuaStack();
    lua_rawgeti(mLuaState, LUA_REGISTRYINDEX, pluginRef);
    lua_getfield(mLuaState, -1, method);

    //sDebugPrint("Function pushed\n");
    //_dumpLuaStack();

    if (!lua_isfunction(mLuaState, -1))
    {
      // TODO: not a function
      break;
    }

    LuaCheck(lua_pcall(mLuaState, 0, 0, 0));

    //sDebugPrint("Function called\n");
    //_dumpLuaStack();

    // Pop the table
    lua_pop(mLuaState, 1);

    //sDebugPrint("After %d\n", pluginRef);
    //_dumpLuaStack();
  }

  printf("Lua stack size: %d\n", lua_gettop(mLuaState));
}


void LuaFXUsermod::_dumpLuaStack()
{
  sDebugPrint("=== Start Stack Dump ===\n");
  int i;
  int top = lua_gettop(mLuaState);
  for (i = 1; i <= top; i++)
  { /* repeat for each level */
    int t = lua_type(mLuaState, i);
    switch (t)
    {

    case LUA_TSTRING: /* strings */
      sDebugPrint("`%s'", lua_tostring(mLuaState, i));
      break;

    case LUA_TBOOLEAN: /* booleans */
      sDebugPrint(lua_toboolean(mLuaState, i) ? "true" : "false");
      break;

    case LUA_TNUMBER: /* numbers */
      sDebugPrint("%g", lua_tonumber(mLuaState, i));
      break;

    default: /* other values */
      sDebugPrint("%s", lua_typename(mLuaState, t));
      break;
    }
    sDebugPrint("\n"); /* put a separator */
  }
  sDebugPrint("\n"); /* end the listing */
  sDebugPrint("=== End Stack Dump ===\n");
}

void LuaFXUsermod::_bindFunctions() {
  ///
  // Segment
  ///

  // set_segment_range(int segmentId, int lightStart, int lightEnd)
  lua_pushcfunction(mLuaState, [](lua_State *L) {
    const int segmentId = luaL_checkinteger(L, 1);
    const int lightStart = luaL_checkinteger(L, 2);
    const int lightEnd = luaL_checkinteger(L, 3);
    sDebugPrint("set_segment_range(%d, %d, %d)\n", segmentId, lightStart, lightEnd);
    strip.setSegment(segmentId, lightStart, lightEnd);
    return 0;
  });
  lua_setglobal(mLuaState, "set_segment_range");

  // set_segment_color(int segmentId, int lightStart, int lightEnd, int color)
  lua_pushcfunction(mLuaState, [](lua_State *L) {
    const int segmentId = luaL_checkinteger(L, 1);
    const int lightStart = luaL_checkinteger(L, 2);
    const int lightEnd = luaL_checkinteger(L, 3);
    const int color = luaL_checkinteger(L, 4);
    sDebugPrint("set_segment_color(%d, %d, %d, %d)\n", segmentId, lightStart, lightEnd, color);

    WS2812FX::Segment& seg = strip.getSegment(static_cast<uint8_t>(segmentId));
    for (int i = lightStart; i <= lightEnd; ++i) {
      seg.colors[i] =color;
    }

    return 0;
  });
  lua_setglobal(mLuaState, "set_segment_color");
}