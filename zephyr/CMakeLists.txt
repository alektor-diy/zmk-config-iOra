if(CONFIG_ZMK_INPUT_PROCESSOR_TEMP_LAYER AND CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
  target_sources(app PRIVATE ../src/iora_auto_mouse.c)
endif()
