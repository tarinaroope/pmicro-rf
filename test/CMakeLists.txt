    
    add_executable (pico-rf-test
                      testmain.c
                  )
    target_include_directories(pico-rf-test PRIVATE ../inc)
    target_link_libraries(pico-rf-test pico_stdlib pmicro-rf)
    
    # enable usb output, disable uart output
    pico_enable_stdio_usb(pico-rf-test 0)
    pico_enable_stdio_uart(pico-rf-test 1)

    # create map/bin/hex/uf2 file etc.
    pico_add_extra_outputs(pico-rf-test)
