ADD_EXECUTABLE(om_make_nerve make_nerve.cpp)
TARGET_LINK_LIBRARIES (om_make_nerve OpenMEEG)

ADD_EXECUTABLE(om_mesh_convert mesh_convert.cpp)
TARGET_LINK_LIBRARIES (om_mesh_convert OpenMEEG)

ADD_EXECUTABLE(om_geometry_info geometry_info.cpp)
TARGET_LINK_LIBRARIES (om_geometry_info OpenMEEG)

ADD_EXECUTABLE(om_mesh_info mesh_info.cpp)
TARGET_LINK_LIBRARIES (om_mesh_info OpenMEEG)

ADD_EXECUTABLE(om_mesh_smooth mesh_smooth.cpp)
TARGET_LINK_LIBRARIES (om_mesh_smooth OpenMEEG)

ADD_EXECUTABLE(om_mesh_concat mesh_concat.cpp)
TARGET_LINK_LIBRARIES (om_mesh_concat OpenMEEG)

ADD_EXECUTABLE(om_project_sensors project_sensors.cpp)
TARGET_LINK_LIBRARIES (om_project_sensors OpenMEEG)

ADD_EXECUTABLE(om_register_squids register_squids.cpp)
TARGET_LINK_LIBRARIES (om_register_squids OpenMEEG)

ADD_EXECUTABLE(om_squids2vtk squids2vtk.cpp)
TARGET_LINK_LIBRARIES (om_squids2vtk OpenMEEGMaths)

ADD_EXECUTABLE(om_matrix_info matrix_info.cpp)
TARGET_LINK_LIBRARIES (om_matrix_info OpenMEEGMaths)

ADD_EXECUTABLE(om_matrix_convert matrix_convert.cpp)
TARGET_LINK_LIBRARIES (om_matrix_convert OpenMEEGMaths)

ADD_EXECUTABLE(om_check_geom check_geom.cpp)
TARGET_LINK_LIBRARIES (om_check_geom OpenMEEG)

ADD_EXECUTABLE(om_mesh_to_dip mesh_to_dip.cpp)
TARGET_LINK_LIBRARIES (om_mesh_to_dip OpenMEEG)

INCLUDE(CgalOption)
IF (USE_CGAL)
    ADD_EXECUTABLE(om_cgal_mesh_3Dlevelset_image cgal_mesh_3Dlevelset_image.cpp)
    TARGET_LINK_LIBRARIES (om_cgal_mesh_3Dlevelset_image OpenMEEG ${CGAL_LIBRARIES})
    ADD_EXECUTABLE(om_cgal_mesh_3Dlabeled_image cgal_mesh_3Dlabeled_image.cpp)
    TARGET_LINK_LIBRARIES (om_cgal_mesh_3Dlabeled_image OpenMEEG ${CGAL_LIBRARIES})
    ADD_EXECUTABLE(om_cgal_mesh_function cgal_mesh_function.cpp)
    TARGET_LINK_LIBRARIES (om_cgal_mesh_function OpenMEEG ${CGAL_LIBRARIES})
    ADD_EXECUTABLE(om_cgal_remesh cgal_remesh.cpp)
    TARGET_LINK_LIBRARIES (om_cgal_remesh OpenMEEG ${CGAL_LIBRARIES})
    INSTALL(TARGETS om_cgal_mesh_3Dlevelset_image om_cgal_mesh_3Dlabeled_image om_cgal_mesh_function om_cgal_remesh DESTINATION bin)
ENDIF()

IF (USE_VTK)
    ADD_EXECUTABLE(om_vtp_to_meshes vtp_to_meshes.cpp)
    TARGET_LINK_LIBRARIES (om_vtp_to_meshes OpenMEEG)

    ADD_EXECUTABLE(om_meshes_to_vtp meshes_to_vtp.cpp)
    TARGET_LINK_LIBRARIES (om_meshes_to_vtp OpenMEEG)

    ADD_EXECUTABLE(om_add_dataset_to_vtk add_dataset_to_vtk.cpp)
    TARGET_LINK_LIBRARIES (om_add_dataset_to_vtk OpenMEEG ${VTK_LIBRARIES})
    INSTALL(TARGETS om_vtp_to_meshes om_meshes_to_vtp om_add_dataset_to_vtk DESTINATION bin)
ENDIF()

INSTALL(TARGETS 
    om_make_nerve
    om_mesh_convert 
    om_mesh_concat
    om_project_sensors
    om_mesh_info om_mesh_smooth om_register_squids
    om_geometry_info
    om_squids2vtk om_matrix_info om_matrix_convert om_check_geom om_mesh_to_dip DESTINATION bin
    )

IF (USE_VTK)
    OPENMEEG_TEST(Tool-om_meshes_to_vtp ${CMAKE_CURRENT_BINARY_DIR}/om_meshes_to_vtp -i1 ${CMAKE_SOURCE_DIR}/data/Models/Head1/cortex.1.tri -i2 ${CMAKE_SOURCE_DIR}/data/Models/Head1/skull.1.tri -i3 ${CMAKE_SOURCE_DIR}/data/Models/Head1/scalp.1.tri -n1 "cortex" -n2 "skull" -n3 "scalp" -o ${OpenMEEG_BINARY_DIR}/tests/Head1.vtp)

    OPENMEEG_TEST(Tool-om_vtp_to_meshes ${CMAKE_CURRENT_BINARY_DIR}/om_vtp_to_meshes -i ${OpenMEEG_BINARY_DIR}/tests/Head1.vtp -o ${OpenMEEG_BINARY_DIR}/tests/meshes.tri)
ENDIF()
