function platform_build_bootimg(){
		gzip -f "${WORKSPACE}/uefi-${DEVICE}-kernel"
        python3 "${ROOTDIR}/tools/mkbootimg.py" \
                --tags_offset 0x0bc08000 \
                --second_offset 0x00e88000 \
                --ramdisk_offset 0x07c08000 \
                --pagesize 2048 \
                --kernel_offset 0x00008000 \
                --header_version 2  \
                --dtb_offset 0x0bc08000 \
                --cmdline "bootopt=64S3,32N2,64N2" \
                --base 0x40078000 \
                --ramdisk "Platform/MediaTek/mt6768/dummyramdisk" \
                --dtb "Platform/MediaTek/mt6768/FdtBlob_compat/merlinx.dtb" \
                --kernel "${WORKSPACE}/uefi-${DEVICE}-kernel.gz" \
                -o "${WORKSPACE}/boot.img" \
                ||return "$?"
}
