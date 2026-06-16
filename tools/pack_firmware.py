"""
固件打包脚本 - 将 APP bin 文件打包成带固件包头的 Ymodem 升级包
用法: python pack_firmware.py app.bin [输出文件]
"""
import sys
import os
import time
import hashlib
import struct

# ============ 固件包头参数（与 bootloader iap_config.h 保持一致）============
MAGIC_NUMBER   = 0x12345678
HEADER_VERSION = 1
START_ADDRESS  = 0x08008200  # APPLICATION_START_ADDR
HEADER_SIZE    = 64          # FIRMWARE_HEADER_SIZE
CHIP_NAME      = b"STM32F1"  # target_chip, 最多 8 字节

# ============ 包头结构体 (64 bytes, packed) ============
# offset  size  field
#   0      4    magic_number
#   4      4    version
#   8      4    firmware_size
#  12     16    firmware_md5
#  28      4    start_address
#  32      4    build_time
#  36     16    version_string
#  52      8    target_chip
#  60      4    reserved


def pack_firmware(input_bin: str, output_bin: str, version_str: str = "v1.0.0"):
    # 读取 APP 固件
    with open(input_bin, "rb") as f:
        firmware_data = f.read()

    firmware_size = len(firmware_data)
    print(f"固件大小: {firmware_size} 字节 ({firmware_size / 1024:.1f} KB)")

    # 计算 MD5
    firmware_md5 = hashlib.md5(firmware_data).digest()
    print(f"MD5:      {firmware_md5.hex().upper()}")

    # 构建包头 (64 bytes)
    header = struct.pack(
        "<I I I 16s I I 16s 8s 4s",
        MAGIC_NUMBER,                            # magic_number
        HEADER_VERSION,                          # version
        firmware_size,                           # firmware_size
        firmware_md5,                            # firmware_md5 (16 bytes)
        START_ADDRESS,                           # start_address
        int(time.time()),                        # build_time (Unix 时间戳)
        version_str.encode("utf-8").ljust(16, b"\x00")[:16],  # version_string
        CHIP_NAME.ljust(8, b"\x00")[:8],         # target_chip
        b"\x00" * 4                              # reserved
    )

    assert len(header) == HEADER_SIZE, f"包头大小错误: {len(header)}"

    # 包头 + 固件 = 完整升级包
    package = header + firmware_data
    with open(output_bin, "wb") as f:
        f.write(package)

    print(f"打包完成: {output_bin}")
    print(f"总大小:   {len(package)} 字节 (包头 64 + 固件 {firmware_size})")
    print()
    print("通过串口助手发送此文件即可升级（SecureCRT → Transfer → Send Ymodem）")


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        print("示例: python pack_firmware.py app.bin")
        print("      python pack_firmware.py app.bin firmware_v2.bin")
        print("      python pack_firmware.py app.bin output.bin v2.0.1")
        sys.exit(1)

    input_bin  = sys.argv[1]
    output_bin = sys.argv[2] if len(sys.argv) > 2 else "firmware_pkg.bin"
    version    = sys.argv[3] if len(sys.argv) > 3 else "v1.0.0"

    if not os.path.exists(input_bin):
        print(f"错误: 文件不存在 - {input_bin}")
        sys.exit(1)

    if len(version.encode("utf-8")) > 16:
        print("错误: 版本字符串超过 16 字节")
        sys.exit(1)

    pack_firmware(input_bin, output_bin, version)


if __name__ == "__main__":
    main()
