#include <sys/time.h>
#include <assert.h>
#include <stdio.h>

int main() {
  struct timeval start, now;
  
  // 1. 获取初始基准时间
  if (gettimeofday(&start, NULL) != 0) {
    assert(0 && "gettimeofday failed");
  }

  uint64_t last_sec = start.tv_sec;

  while (1) {
    // 2. 持续获取当前时间
    gettimeofday(&now, NULL);

    // 3. 检查秒数是否发生变化
    // 或者计算微秒差值：if ((now.tv_sec - start.tv_sec) >= 1)
    if (now.tv_sec > last_sec) {
      printf("One second passed! Current Time: %ld s\n", (long)now.tv_sec);
      
      // 4. 更新基准，准备下一次计时
      last_sec = now.tv_sec; 
    }

    // 可以在这里加一个非常小的 yield 或延迟，避免 100% 占用 CPU
  }

  return 0;
}