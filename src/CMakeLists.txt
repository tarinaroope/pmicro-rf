    
    add_library (   pmicro-rf
                    rx_device.c
                    tx_device.c
                    rf_pico.c
                    pico_synchronizer.c
                )
    target_include_directories(pmicro-rf PUBLIC ../inc)

    if (${PICO_BOARD} STREQUAL "pico_w")
        target_compile_definitions(pmicro-rf PRIVATE USING_PICO_W)
        target_link_libraries(pmicro-rf pico_stdlib pico_cyw43_arch_none)
    else()
        target_compile_definitions(pmicro-rf PRIVATE USING_PICO)
        target_link_libraries(pmicro-rf pico_stdlib)
    endif()

    pico_enable_stdio_usb(pmicro-rf 0)
    pico_enable_stdio_uart(pmicro-rf 1)