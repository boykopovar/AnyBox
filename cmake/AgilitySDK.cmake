set(ANYBOX_AGILITY_SDK_VERSION 614)
set(ANYBOX_AGILITY_SDK_CACHE "${PROJECT_SOURCE_DIR}/build/deps/agility" CACHE PATH "Microsoft Agility SDK package cache")
set(agilityPackage "${ANYBOX_AGILITY_SDK_CACHE}/Microsoft.Direct3D.D3D12.1.614.1.nupkg")
set(agilityHash 9880aa91602dd51dd6cf7911a2bca7a2323513b15338573cde014b3356eeaff2)
file(MAKE_DIRECTORY "${ANYBOX_AGILITY_SDK_CACHE}")
if(NOT EXISTS "${agilityPackage}")
    file(DOWNLOAD "https://api.nuget.org/v3-flatcontainer/microsoft.direct3d.d3d12/1.614.1/microsoft.direct3d.d3d12.1.614.1.nupkg" "${agilityPackage}" EXPECTED_HASH "SHA256=${agilityHash}" TLS_VERIFY ON)
endif()
file(SHA256 "${agilityPackage}" actualAgilityHash)
if(NOT actualAgilityHash STREQUAL agilityHash)
    message(FATAL_ERROR "Microsoft Agility SDK package checksum does not match")
endif()

set(agilityCore "${ANYBOX_AGILITY_SDK_CACHE}/build/native/bin/x64/D3D12Core.dll")
set(agilityLayers "${ANYBOX_AGILITY_SDK_CACHE}/build/native/bin/x64/d3d12SDKLayers.dll")
set(agilityLicense "${ANYBOX_AGILITY_SDK_CACHE}/LICENSE.txt")
if(NOT EXISTS "${agilityCore}" OR NOT EXISTS "${agilityLayers}" OR NOT EXISTS "${agilityLicense}")
    file(ARCHIVE_EXTRACT INPUT "${agilityPackage}" DESTINATION "${ANYBOX_AGILITY_SDK_CACHE}" PATTERNS "build/native/bin/x64/D3D12Core.dll" "build/native/bin/x64/d3d12SDKLayers.dll" "LICENSE.txt")
endif()
file(SHA256 "${agilityCore}" coreHash)
file(SHA256 "${agilityLayers}" layersHash)
file(SHA256 "${agilityLicense}" licenseHash)
if(NOT coreHash STREQUAL "8a23d826b25b4329522ff451cb52b7f2b34d7f2913cfeb878371ce8bd765fe2d" OR NOT layersHash STREQUAL "10a1bb05c914ff8dcea45b50166759edb970677e9458c34a1a99f7e0aee7be87" OR NOT licenseHash STREQUAL "5239850894610071566f7ecee0b751fde43c862032d92b99d7d0f596b3433ebd")
    message(FATAL_ERROR "Microsoft Agility SDK runtime files have been modified")
endif()

install(FILES "${agilityCore}" "${agilityLayers}" "${agilityLicense}" DESTINATION D3D12 COMPONENT AnyBoxRuntime)

add_custom_target(anybox_d3d12_runtime
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${ANYBOX_RUNTIME_DIRECTORY}/D3D12"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${agilityCore}" "${ANYBOX_RUNTIME_DIRECTORY}/D3D12/D3D12Core.dll"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${agilityLayers}" "${ANYBOX_RUNTIME_DIRECTORY}/D3D12/d3d12SDKLayers.dll"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${agilityLicense}" "${ANYBOX_RUNTIME_DIRECTORY}/D3D12/LICENSE.txt"
    BYPRODUCTS "${ANYBOX_RUNTIME_DIRECTORY}/D3D12/D3D12Core.dll" "${ANYBOX_RUNTIME_DIRECTORY}/D3D12/d3d12SDKLayers.dll" "${ANYBOX_RUNTIME_DIRECTORY}/D3D12/LICENSE.txt"
    VERBATIM
)
