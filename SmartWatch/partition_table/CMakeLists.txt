idf_component_register(SRCS "partitions.csv"
                       INCLUDE_DIRS "."
                       KCONFIG "Kconfig.projbuild")

# 添加自定义目标用于生成分区表
partition_table_get_args(pt_args --partition-table "${CMAKE_CURRENT_SOURCE_DIR}/partitions.csv")
partition_table_target(pt_target "${pt_args}")

# 添加依赖，确保分区表在构建应用前生成
add_dependencies(${COMPONENT_LIB} pt_target)