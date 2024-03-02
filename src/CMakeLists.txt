    
    add_library (   pmicro-rf
                    rf_device.c
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

    # enable usb output, disable uart output
    pico_enable_stdio_usb(pmicro-rf 0)
    pico_enable_stdio_uart(pmicro-rf 1)

    # create map/bin/hex/uf2 file etc.
   # pico_add_extra_outputs(pmicro-rf)

    # add url via pico_set_program_url
 #   example_auto_set_url(pmicro-rf)
