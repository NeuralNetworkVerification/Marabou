set(run_regress_script ${CMAKE_CURRENT_LIST_DIR}/run_regression.py)
macro(marabou_add_regress_test level net_file property_file result arguments
         test_properties)
    get_filename_component(net_name ${net_file} NAME)
    get_filename_component(prop_name ${property_file} NAME)
    set(test_name "${net_name}%${prop_name}%${arguments}")
    add_test(${test_name}
            ${run_regress_script}
            ${CMAKE_BINARY_DIR}/${MARABOU_EXE}
                ${net_file}
                ${property_file}
                ${result}
                ${arguments})
    set_tests_properties(${test_name} PROPERTIES LABELS "regress${level}
    ${result} ${test_properties}")
endmacro()

macro(marabou_add_twin_test level net_file result)
    marabou_add_regress_test(${level} ${net_file}
        "${CMAKE_SOURCE_DIR}/resources/properties/builtin_property.txt" ${result} "" "twin")
endmacro()

macro(marabou_add_coav_test level net_file result)
    marabou_add_regress_test(${level} 
        "${CMAKE_SOURCE_DIR}/resources/nnet/coav/${net_file}"
        "${CMAKE_SOURCE_DIR}/resources/properties/builtin_property.txt" "${result}" "" "coav")
endmacro()

macro(marabou_add_acasxu_test level net_file prop_num result)
    marabou_add_regress_test(${level} 
        "${CMAKE_SOURCE_DIR}/resources/nnet/acasxu/${net_file}"
        "${CMAKE_SOURCE_DIR}/resources/properties/acas_property_${prop_num}.txt" "${result}" "" "acasxu")
endmacro()

macro(marabou_add_acasxu_dnc_test level net_file prop_num result)
    marabou_add_regress_test(${level} 
        "${CMAKE_SOURCE_DIR}/resources/nnet/acasxu/${net_file}"
        "${CMAKE_SOURCE_DIR}/resources/properties/acas_property_${prop_num}.txt" "${result}" "--dnc" "acasxu")
endmacro()

            
macro(marabou_add_regress_mps_test level net_file result arguments
         test_properties)
    get_filename_component(net_name ${net_file} NAME)
    set(test_name "${net_name}")
    add_test(${test_name}
            ${run_regress_script}
                ${CMAKE_BINARY_DIR}/${INPUT_PARSERS_DIR}/${MPS_PARSER}
                "${CMAKE_SOURCE_DIR}/${net_file}"
                ${result}
                ${arguments})
    set_tests_properties(${test_name} PROPERTIES LABELS "regress${level}
    ${result} ${test_properties}")
endmacro()

add_subdirectory(regress0)
add_subdirectory(regress1)
add_subdirectory(regress2)
add_subdirectory(regress3)
add_subdirectory(regress4)
add_subdirectory(regress5)

