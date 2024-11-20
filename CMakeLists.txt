cmake_minimum_required(VERSION 3.10)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(cppos ASM CXX)

set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")
add_compile_options("-ffreestanding" "-fno-rtti" "-fno-exceptions")
add_compile_definitions("_LIBCPP_HAS_NO_THREADS")
add_link_options("-nostdlib" "-ffreestanding" )

set(ARCH "arm" CACHE STRING "Architecture to build for, supported values: [arm]")
message("Building for architecture: ${ARCH}")

if(${ARCH} STREQUAL "arm")
    set(TARGET_TRIPLE "arm-none-eabi")
    set(CMAKE_ASM_COMPILER_TARGET ${TARGET_TRIPLE})
    set(CMAKE_C_COMPILER_TARGET ${TARGET_TRIPLE})
    set(CMAKE_CXX_COMPILER_TARGET ${TARGET_TRIPLE})
    add_link_options("-mcpu=cortex-a7")
    add_compile_options("-mcpu=cortex-a7" "-mno-unaligned-access")
else()
    message(FATAL_ERROR "Unsupported architecture: ${ARCH}")
endif()

include_directories(/usr/include/newlib)
include_directories(/usr/include/newlib/c++/13.2.1/)
include_directories(/usr/include/newlib/c++/13.2.1/${TARGET_TRIPLE}/)

set(ARCH_SOURCES_arm
    "arch/arm/cpu.cpp"
    "arch/arm/entry.S"
    "arch/arm/interrupts.cpp"
    "arch/arm/interrupt_table.S"
    "arch/arm/interrupt_trampolines.S"
)
set(SOURCES
    "drivers/gpio.cpp"
    "drivers/interrupt_controller.cpp"
    "drivers/serial.cpp"
    "drivers/timer.cpp"
    "drivers/watchdog.cpp"
    "kernel/basic.cpp"
    "kernel/c++support.cpp"
    "kernel/events.cpp"
    "kernel/exceptions.cpp"
    "kernel/images.cpp"
    "kernel/memory.cpp"
    "kernel/supervisor.cpp"
    "kernel/start.cpp"
    "kernel/threads.cpp"
    "lib/format.cpp"
    "lib/string.cpp"
)
set(LINKER_SCRIPT "kernel.lds")
set(LINKER_SCRIPT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${LINKER_SCRIPT})

add_executable(kernel ${SOURCES} ${ARCH_SOURCES_${ARCH}})
target_include_directories(kernel PRIVATE "include/")
target_compile_features(kernel PRIVATE cxx_std_23)
set_target_properties(kernel PROPERTIES LINK_DEPENDS ${LINKER_SCRIPT_PATH})
target_link_options(kernel PRIVATE "-T${LINKER_SCRIPT_PATH}")
set_target_properties(kernel PROPERTIES SUFFIX ".elf")
