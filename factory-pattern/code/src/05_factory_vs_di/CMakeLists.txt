# 第 5 篇《工厂和依赖注入，到底谁管什么》配套代码

# 工厂侧：工厂 + 具体渠道实现，打成一个库。
# 单独成库，是为了让测试目标可以选择「不链接它」。
add_library(fp_05_factory STATIC
    payment_factory.cpp
    processors.cpp)
target_include_directories(fp_05_factory PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(fp_05_factory PUBLIC fp_core)

# 演示程序：组合根在这个可执行文件里
add_executable(fp_05_main
    main.cpp
    order_service.cpp
    service_locating.cpp)
target_link_libraries(fp_05_main PRIVATE fp_05_factory)

# 单元测试：只链接 order_service.cpp。
# 注意它 **没有** 链接 fp_05_factory —— 因为 OrderService 不需要工厂。
# 这不是省略，这是一句可以编译的断言。
add_executable(fp_05_test_order_service
    test_order_service.cpp
    order_service.cpp)
target_include_directories(fp_05_test_order_service PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(fp_05_test_order_service PRIVATE fp_core)

add_test(NAME 05_factory_vs_di COMMAND fp_05_test_order_service)
