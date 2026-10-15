set_xmakever("2.8.2")

add_requires("nlohmann_json")

includes("lib/commonlibsse-ng")

set_project("OutfitWheeler-DynamicEquipmentManager")
set_version("3.0")
set_license("GPL-3.0")

set_languages("c++23")
set_warnings("allextra")

set_policy("package.requires_lock", true)

add_rules("mode.release")
add_rules("plugin.vsxmake.autoupdate")

target("OutfitWheeler-DynamicEquipmentManager")
    add_deps("commonlibsse-ng")
    add_packages("nlohmann_json")

    add_rules("commonlibsse-ng.plugin", {
        name = "Outfit Wheeler - Dynamic Equipment Manager",
        author = "StarkMP <discord: starkmp>",
        description = "SKSE64 plugin template using CommonLibSSE-NG and PrismaUI"
    })

    add_files("src/**.cpp")
    add_headerfiles("src/**.h")
    add_includedirs("src")
    set_pcxxheader("src/pch.h")

target("OutfitWheeler-BackendTests")
    set_kind("binary")
    set_default(false)
    add_packages("nlohmann_json")
    add_files("tests/test_backend.cpp")
    add_files("src/Settings.cpp")
    add_files("src/thumbnail/ThumbnailDiskCache.cpp")
    add_files("src/ModCatalogManager.cpp")
    add_includedirs("src", "src/thumbnail")
    add_syslinks("windowscodecs", "ole32")