#pragma once

#include "wled.h"
#include <vector>

struct lua_State;

class LuaFXUsermod : public Usermod
{
  public:
  LuaFXUsermod();

  virtual void loop() override;
  virtual void setup() override;
  virtual void connected() override;
  virtual void addToJsonState(JsonObject &obj) override;
  virtual void addToJsonInfo(JsonObject &obj) override;
  virtual void readFromJsonState(JsonObject &obj) override;
  virtual uint16_t getId() override { return USERMOD_ID_UNSPECIFIED; }

  private:
    void _callOnAllPlugins(const char* method);
    void _dumpLuaStack();
    void _bindFunctions();

  lua_State* mLuaState;
  std::vector<int> mPluginRefs;
  long mLastTime;
};