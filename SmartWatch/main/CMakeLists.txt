
# HDL
file(GLOB_RECURSE HDL_SRCS
	hdl/bt/*.c
	hdl/bt/*.cpp
)
set(HDL_INCS
    hdl/
	hdl/button/
	hdl/disp/
	hdl/lvgl/
	hdl/power/
	hdl/wifi/
	hdl/bt/
	hdl/audio/
	hdl/flash/
	hdl/imu/
)

# FML
file(GLOB_RECURSE FML_SRCS
	fml/HdlManager/*.c
	fml/HdlManager/*.cpp
	fml/JpegDecoder/*.c
	fml/JpegDecoder/*.cpp
	fml/SpeechRecongnition/*.c
	fml/SpeechRecongnition/*.cpp
	fml/TextToSpeech/*.c
	fml/TextToSpeech/*.cpp
	fml/BigModel/*.c
	fml/BigModel/*.cpp
)
set(FML_INCS
	fml/
	fml/HdlManager/
	fml/JpegDecoder/
	fml/SpeechRecongnition/
	fml/TextToSpeech/
	fml/BigModel/
)

# BLL
file(GLOB_RECURSE BLL_SRCS
	bll/LvglFont/*.c
	bll/LvglImg/*.c
	bll/ArtificialIntelligence/*.c
	bll/ArtificialIntelligence/*.cpp
)
set(BLL_INCS
	bll/
	bll/ArtificialIntelligence/
)

# APL
file(GLOB_RECURSE APL_SRCS
	apl/*.c
    apl/*.cpp
    apl/WatchDial/*.c
    apl/WatchDial/*.cpp
	apl/AppStore/*.c
    apl/AppStore/*.cpp
	apl/Assistant/*.c
    apl/Assistant/*.cpp
	apl/Painter/*.c
    apl/Painter/*.cpp
	apl/Setting/*.c
    apl/Setting/*.cpp
	apl/GamePad/*.c
    apl/GamePad/*.cpp
)
set(APL_INCS
    apl/
	apl/WatchDial/
	apl/AppStore/
	apl/Assistant/
	apl/Painter/
	apl/Setting/
	apl/GamePad/
)

# 添加分区表目录到组件搜索路径
list(APPEND EXTRA_COMPONENT_DIRS partition_table)

# 指定自定义分区表文件
set(PARTITION_TABLE_FILE "partition_table/partitions.csv")



idf_component_register(SRCS "main.cpp" ${HDL_SRCS} ${FML_SRCS} ${BLL_SRCS} ${APL_SRCS}
                    INCLUDE_DIRS "." ${HDL_INCS} ${FML_INCS} ${BLL_INCS} ${APL_INCS})
					
add_definitions(-w)

# Determine whether esp-sr is fetched from component registry or from local path
idf_build_get_property(build_components BUILD_COMPONENTS)
if(esp-sr IN_LIST build_components)
    set(esp-sr_name esp-sr) # Local component
elseif(esp-sr-master IN_LIST build_components)
    # 处理 GitHub 下载的 master 分支情况
    set(esp-sr_name esp-sr-master)	
else()
    set(esp-sr_name espressif__esp-sr) # Managed component
endif()

# Get path to voice data file
idf_component_get_property(esp-sr_path ${esp-sr_name} COMPONENT_DIR)
set(voice_data_image ${esp-sr_path}/esp-tts/esp_tts_chinese/esp_tts_voice_data_xiaoxin_small.dat)
add_custom_target(voice_data ALL DEPENDS ${voice_data_image})
add_dependencies(flash voice_data)

partition_table_get_partition_info(size "--partition-name voice_data" "size")
partition_table_get_partition_info(offset "--partition-name voice_data" "offset")

if("${size}" AND "${offset}")
    esptool_py_flash_to_partition(flash "voice_data" "${voice_data_image}")
else()
    set(message "Failed to find model in partition table file"
                "Please add a line(Name=voice_data, Type=data, Size=3890K) to the partition file.")
endif()					
					
#将图片打包进文件系统,不能放在idf_component_register上面
littlefs_create_partition_image(littlefs ../images FLASH_IN_PROJECT)
