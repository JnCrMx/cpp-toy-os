#pragma once

namespace kernel::driver::watchdog {

[[noreturn]] void restart();
[[noreturn]] void poweroff();

}
