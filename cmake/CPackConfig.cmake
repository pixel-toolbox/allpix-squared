# Packaging configuration
SET(CPACK_PACKAGE_NAME "allpix-squared")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Generic Pixel Detector Simulation Framework")
SET(CPACK_PACKAGE_VENDOR "The Allpix Squared Authors")
SET(CPACK_PACKAGE_CONTACT "The Allpix Squared Authors <allpix.squared@cern.ch>")
SET(CPACK_PACKAGE_ICON "doc/logo_small.png")
SET(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE.md")

# Set version dervied from project version:
SET(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
IF("${PROJECT_VERSION_PATCH}" STREQUAL "")
    SET(CPACK_PACKAGE_VERSION_PATCH "0")
ELSE()
    SET(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
ENDIF()

# Configure the targets and components to include
SET(CPACK_GENERATOR "TGZ")
SET(CPACK_COMPONENTS_ALL application modules)
