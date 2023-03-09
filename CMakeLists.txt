project(bilibili-spider
        DESCRIPTION "哔哩哔哩爬虫")

cmake_minimum_required(VERSION 3.2)

add_subdirectory(third-part)

include_directories(third-part)

add_executable(main main.cpp)

target_link_libraries(main
                        http)
