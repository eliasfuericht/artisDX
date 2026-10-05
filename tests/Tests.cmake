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

list(SORT ARTISDX_SHADERS)
foreach(mode debug optimized)
    artisDX_add_test(shaders_${mode} shaders shaders ${mode} ${ARTISDX_SHADERS})
endforeach()

artisDX_add_test(shader_failure_handling shaders shader-failures "${PROJECT_SOURCE_DIR}/tests/fixtures")
artisDX_add_test(camera_constants_readback gpu constants)
artisDX_add_test(command_failure_handling gpu commands)
artisDX_add_test(root_signature_failure_handling gpu root-failures)
artisDX_add_test(bounding_box_transforms gpu bounds)
artisDX_add_test(rtv_descriptor_spacing gpu rtv)
option(ARTISDX_TEST_COMPATIBILITY "Test inactive basic/normal example pipeline compatibility" OFF)
if(ARTISDX_TEST_COMPATIBILITY)
    foreach(pass basic normal)
        artisDX_add_test(pipeline_${pass} "gpu;compatibility" pipeline ${pass})
    endforeach()
endif()
artisDX_add_test(texture_copy_upload gpu upload "${PROJECT_SOURCE_DIR}/tests/fixtures/regression.glb")
artisDX_add_test(glb_render_regression gpu render "${PROJECT_SOURCE_DIR}/tests/fixtures/regression.glb")

artisDX_add_test(reflected_binding_layout gpu root-layout "${PROJECT_SOURCE_DIR}/tests/fixtures")

artisDX_add_test(command_submission_completion gpu completion)

artisDX_add_test(submission_resource_retirement gpu retirement "${PROJECT_SOURCE_DIR}/tests/fixtures/regression.glb")

artisDX_add_test(pass_state_and_transforms gpu passes "${PROJECT_SOURCE_DIR}/tests/fixtures/regression.glb")
