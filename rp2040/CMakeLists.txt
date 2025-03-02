add_library (pmicro-rf
            ../src/rx_device.c
            ../src/tx_device.c
            ../rp2040/rf_pico.c
            ../rp2040/pico_synchronizer.c
            )
target_include_directories(pmicro-rf PUBLIC ../inc ../src ../rp2040)

# Keeping this here in case variation is needed. Now it's useless though.
if (${PICO_BOARD} STREQUAL "pico_w")
    target_compile_definitions(pmicro-rf PRIVATE USING_PICO_W)
    target_link_libraries(pmicro-rf pico_stdlib)
else()
    target_compile_definitions(pmicro-rf PRIVATE USING_PICO)
    target_link_libraries(pmicro-rf pico_stdlib)
endif()

pico_enable_stdio_usb(pmicro-rf 0)
pico_enable_stdio_uart(pmicro-rf 1)