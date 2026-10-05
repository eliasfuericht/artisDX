# The command-line hook also provides the BUILD_TESTING=OFF explanation.
target_sources(${APPLICATION_NAME} PRIVATE tests/TestLauncher.cpp tests/TestLauncher.h)
target_include_directories(${APPLICATION_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/tests")
source_group("tests" FILES tests/TestLauncher.cpp tests/TestLauncher.h)
if(NOT BUILD_TESTING)
    return()
endif()

set(ARTISDX_TEST_SOURCES
    tests/TestRunner.cpp
    tests/CpuTests.cpp
    tests/ShaderTests.cpp
    tests/GpuTests.cpp
    tests/TestCases.h
    tests/TestSupport.h
)
target_sources(${APPLICATION_NAME} PRIVATE ${ARTISDX_TEST_SOURCES})
source_group(TREE "${PROJECT_SOURCE_DIR}/tests" PREFIX "tests" FILES ${ARTISDX_TEST_SOURCES})

# Test cases run through the same executable as the editor.
target_compile_definitions(${APPLICATION_NAME} PRIVATE
    ARTISDX_TESTING_ENABLED
    "ARTISDX_CTEST_PATH=L\"${CMAKE_CTEST_COMMAND}\""
    "ARTISDX_TEST_BUILD_DIR=L\"${PROJECT_BINARY_DIR}\""
    "ARTISDX_TEST_CONFIG=L\"$<CONFIG>\""
)

# Each case runs in its own process: engine globals cannot leak between tests.
function(artisDX_add_test name label)
    add_test(NAME ${name} COMMAND ${APPLICATION_NAME} -test-case ${ARGN})
    set_tests_properties(${name} PROPERTIES
        WORKING_DIRECTORY "${PROJECT_BINARY_DIR}"
        LABELS "${label}"
        TIMEOUT 120
        SKIP_RETURN_CODE 77
    )
endfunction()

artisDX_add_test(camera_controls cpu camera)
artisDX_add_test(mesh_tangent_basis cpu tangents)

foreach(shader IN LISTS ARTISDX_SHADERS)
    get_filename_component(shader_name "${shader}" NAME_WE)
    foreach(mode debug optimized)
        artisDX_add_test(shader_${shader_name}_${mode} shaders shader "${shader}" ${mode})
    endforeach()
endforeach()

artisDX_add_test(camera_constants_readback gpu constants)
artisDX_add_test(bounding_box_transforms gpu bounds)
artisDX_add_test(rtv_descriptor_spacing gpu rtv)
foreach(pass pbr dShadowMap bb basic normal)
    artisDX_add_test(pipeline_${pass} gpu pipeline ${pass})
endforeach()
artisDX_add_test(texture_copy_upload gpu upload "${PROJECT_SOURCE_DIR}/tests/fixtures/regression.glb")
artisDX_add_test(glb_render_smoke gpu render "${PROJECT_SOURCE_DIR}/tests/fixtures/regression.glb")
# Keep known defects visible: this is a normal failing test, not WILL_FAIL or DISABLED.
set_property(TEST glb_render_smoke PROPERTY LABELS "gpu;known_failure")
