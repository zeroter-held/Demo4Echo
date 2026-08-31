# Specify the cross-compilation toolchain
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

if(DEFINED ENV{RV1106_SDK_PATH})
    file(TO_CMAKE_PATH "$ENV{RV1106_SDK_PATH}" SDK_PATH)
else()
    set(SDK_PATH "/home/zxx360zxx/Projects/Echo-Mate/SDK/rv1106-sdk")
endif()
set(TOOLCHAIN_PATH ${SDK_PATH}/tools/linux/toolchain/arm-rockchip830-linux-uclibcgnueabihf)

# Specify the compiler paths
set(CMAKE_C_COMPILER ${TOOLCHAIN_PATH}/bin/arm-rockchip830-linux-uclibcgnueabihf-gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PATH}/bin/arm-rockchip830-linux-uclibcgnueabihf-g++)

# Specify the sysroot (if available)
set(CMAKE_SYSROOT ${TOOLCHAIN_PATH}/arm-rockchip830-linux-uclibcgnueabihf/sysroot)

# Add paths to find libraries and includes
set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
