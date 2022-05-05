
#include "exec.h"

namespace jar {

uint32_t exec::name_idx = 0;
uint32_t queuer::name_idx = 0;
uint32_t delayer::name_idx = 0;
uint32_t looper::name_idx = 0;
uint32_t animator::name_idx = 0;

cached_pool pool(0);

}

