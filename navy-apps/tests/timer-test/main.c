#include <assert.h>
#include <stdio.h>

int main() {
  int init_ret = NDL_Init(0);
  uint32_t last_time_ms = NDL_GetTicks();
  assert(init_ret == 0 && last_time_ms != -1 && "NDLInit failed");

  while(1) {
    if(NDL_GetTicks() - last_time_ms < 500) continue;
    printf("Waiting for 0.5 second to pass... Current Time: %d ms\n"\
    , NDL_GetTicks());
    last_time_ms = NDL_GetTicks();
  }

//   while (1) {
//     int ret = gettimeofday(&now, NULL);
//     assert(ret == 0 && "gettimeofday failed");
//     if (now.tv_usec == 500000) {
//       printf("One second passed! Current Time: %ld s\n", (long)now.tv_sec);
//     }
//     // 可以在这里加一个非常小的 yield 或延迟，避免 100% 占用 CPU
//   }

  return 0;
}