// Main c++ test file for samsung_ac external component.

// This is the main test file for the samsung_ac component.
// See https://interrupt.memfault.com/blog/unit-testing-basics
///
// These tests use the Unity test framework.
//   https://docs.platformio.org/en/stable/advanced/unit-testing/frameworks/unity.html
//   https://registry.platformio.org/libraries/throwtheswitch/Unity
//
// TODO: Split tests with multiple assertions into their own test.
//
// FIX: ArduinoMock is throwing error when testing in native mode.
// https://github-wiki-see.page/m/Task-Tracker-Systems/Task-Tracker-Device/wiki/tipps-for-using-FakeIt


#ifndef IS_NATIVE
#error "Must define IS_NATIVE in platformio.ini, either 1 or 0"
#endif

#include <unity.h>

#include <test_samsung_proto.h>


namespace esphome
{
  namespace samsung_ac
  {

  } // esphome
} // samsung_ac

// For native dev-platform or for some embedded frameworks
// int main(void) {
int main(int argc, char **argv)
{
  return esphome::samsung_ac::RunSamsungProtoTests();
}

// For Arduino framework
void setup()
{
  esphome::samsung_ac::RunSamsungProtoTests();
}

void loop() {}
