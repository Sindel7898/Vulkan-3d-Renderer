cmake_minimum_required(VERSION 3.16)
project(Spark)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include common macros file (check if path is correct)
include(${CMAKE_SOURCE_DIR}/CommonMacros.txt)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/External/imgui)
set(ImGuizmo_DIR ${CMAKE_SOURCE_DIR}/External/ImGuizmo)

# ImGui source files (since there's no CMakeLists.txt in imgui directory)
set(IMGUI_SRC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp 
)

set(ImGuizmo_SRC
    ${ImGuizmo_DIR}/GraphEditor.cpp
    ${ImGuizmo_DIR}/ImCurveEdit.cpp
    ${ImGuizmo_DIR}/ImGradient.cpp
    ${ImGuizmo_DIR}/ImGuizmo.cpp
    ${ImGuizmo_DIR}/ImSequencer.cpp  
)

set(IMGUI_BACKEND_SRC
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
)

set(TRACY_SOURCES ${CMAKE_SOURCE_DIR}/External/tracy/public/TracyClient.cpp)

# Add the libs subdirectory
add_subdirectory(External/glfw)
add_subdirectory(External/glm)
add_subdirectory(External/vk-bootstrap)
add_subdirectory(External/VulkanMemoryAllocator)
add_subdirectory(External/tracy)
add_subdirectory(External/fastgltf)
add_subdirectory(External/meshoptimizer)

# Find all source files
file(GLOB SOURCES "Source/*.cpp" "Source/*.c")

# Add Vulkan
find_package(Vulkan REQUIRED)

# Create the executable
add_executable(Spark ${SOURCES}
                              ${TRACY_SOURCES} 
                              ${IMGUI_SRC} 
                              ${IMGUI_BACKEND_SRC} 
                              ${ImGuizmo_SRC})

target_include_directories(Spark PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
    External/src/VulkanMemoryAllocator
    External/src/imgui
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    External/ImGuizmo
    ${ImGuizmo_DIR}
    ${Vulkan_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/External/glfw/include
    ${CMAKE_SOURCE_DIR}/External/tracy/public/tracy
)
target_compile_definitions(Spark PRIVATE TRACY_ENABLE)

target_link_libraries(Spark PRIVATE 
    glfw 
    glm 
    vk-bootstrap 
    VulkanMemoryAllocator
    fastgltf
    meshoptimizer
    Vulkan::Vulkan
)