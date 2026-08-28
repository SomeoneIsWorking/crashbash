#include "crashbash_runtime.h"

#include "testutil.h"

static void test_crash_bash_factory_selects_authored_face_order() {
  const RenderCapabilities capabilities = crashbash::CrashBashRuntime::titleRenderCapabilities();
  CHECK_EQ(capabilities.defaultFaceOrder, FACE_ORDER_AUTHORED);
  CHECK(capabilities.nativeRenderPath);
  CHECK(capabilities.temporalInterpolation);
}

int main() {
  RUN(crash_bash_factory_selects_authored_face_order);
  return pt_summary();
}
