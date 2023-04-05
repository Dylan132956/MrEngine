
VS_VERSION = 'vs2013'

IS_VS = false
IS_XCODE = false
IS_ANDROID = false
IS_LINUX = false
IS_MAC = false
IS_IOS = false

IS_PC_YY = false
IS_PC_AMUSEMENT = false
IS_PC_MSHOW = false
IS_PC_YY_3DMESH = false
IS_AI_CHECK = false
if _ACTION then
    local vsIndex, _ = _ACTION:find('vs')
    if vsIndex == 1 then
        IS_VS = true
    end

    local xcodeIndex, _ = _ACTION:find('xcode')
    if xcodeIndex == 1 then
        IS_XCODE = true
    end

    local cmakeIndex, _ = _ACTION:find('cmake')

    if _OPTIONS["with-linux"] then
        IS_LINUX = (_OPTIONS["with-linux"] == 'yes')
    end
    if cmakeIndex == 1 and IS_LINUX == false then
        IS_ANDROID = true
    end
	
	if _OPTIONS["pc_yy_demo"] then
        IS_PC_YY = (_OPTIONS["pc_yy_demo"] == 'yes')
    end
	
	if _OPTIONS["pc_amusement_demo"] then
        IS_PC_AMUSEMENT = (_OPTIONS["pc_amusement_demo"] == 'yes')
    end
	
	if _OPTIONS["pc_mshow_demo"] then
        IS_PC_MSHOW = (_OPTIONS["pc_mshow_demo"] == 'yes')
    end

    if _OPTIONS["pc_yy_3dmesh_demo"] then
        IS_PC_YY_3DMESH = (_OPTIONS["pc_yy_3dmesh_demo"] == 'yes')
    end
	
	if _OPTIONS["of_designer_demo"] then
        IS_OF_DESIGNER = (_OPTIONS["of_designer_demo"] == 'yes')
    end

    if _OPTIONS["ai_check"] then
        IS_AI_CHECK = (_OPTIONS["ai_check"] == 'yes')
    end

end

function toolchain()

    newoption {
        trigger = "vs",
        value = "toolset",
        description = "Choose VS toolset",
        allowed = {
            { "vs2013-xp",     "Visual Studio 2013 targeting XP" },
            { "vs2015-xp",     "Visual Studio 2015 targeting XP" },
            { "vs2017-xp",     "Visual Studio 2017 targeting XP" },
        },
    }

    newoption {
        trigger = "xcode",
        value = "xcode_target",
        description = "Choose XCode target",
        allowed = {
            { "osx", "OSX" },
            { "ios", "iOS" },
            { "tvos", "tvOS" },
        }
    }

    newoption {
        trigger     = "with-ios",
        value       = "#",
        description = "Set iOS target version (default: 8.0).",
    }

    newoption {
        trigger     = "with-macos",
        value       = "#",
        description = "Set macOS target version (default 10.11).",
    }

    newoption {
        trigger     = "with-tvos",
        value       = "#",
        description = "Set tvOS target version (default: 9.0).",
    }

    newoption {
        trigger = "with-windows",
        value = "#",
        description = "Set the Windows target platform version (default: $WindowsSDKVersion or 8.1).",
    }

    newoption {
        trigger = "with-designer",
        value = "#",
        description = "Whether to generate of designer (default: false).",
    }

    newoption {
        trigger = "with-qt",
        value = "#",
        description = "QT msvc directory (default: C:/Qt/5.15.2/msvc2019/).",
    }

    newoption {
        trigger = "demo",
        value = "#",
        description = "Set android demo.",
    }
	
	newoption {
        trigger = "pc_yy_demo",
        value = "#",
        description = "pc yy demo.",
    }
	
	newoption {
        trigger = "pc_amusement_demo",
        value = "#",
        description = "pc amusement demo.",
    }
	
	newoption {
        trigger = "pc_mshow_demo",
        value = "#",
        description = "pc mshow demo.",
    }

    newoption {
        trigger = "with-linux",
        value = "#",
        description = "Gen linux cmake.",
    }

    newoption {
        trigger = "pc_yy_3dmesh_demo",
        value = "#",
        description = "pc yy 3dmesh demo.",
    }
	
	newoption {
        trigger = "of_designer_demo",
        value = "#",
        description = "pc of_designer demo.",
    }

    newoption {
        trigger = "ai_check",
        value = "#",
        description = "only ai check, no of rendering.",
    }

    print('==> _ACTION=' .. tostring(_ACTION))

    for k, v in pairs(_OPTIONS) do
        print('==> _OPTIONS[' .. k .. ']=' .. v)
    end

    local iosPlatform = ""
    if _OPTIONS["with-ios"] then
        iosPlatform = _OPTIONS["with-ios"]
    end

    local macosPlatform = ""
    if _OPTIONS["with-macos"] then
        macosPlatform = _OPTIONS["with-macos"]
    end

    local tvosPlatform = ""
    if _OPTIONS["with-tvos"] then
        tvosPlatform = _OPTIONS["with-tvos"]
    end

    local windowsPlatform = nil -- string.gsub(os.getenv("WindowsSDKVersion") or "8.1", "\\", "")

    if _OPTIONS["with-windows"] then
        windowsPlatform = _OPTIONS["with-windows"]
    end


    if _ACTION == "vs2013"
        or _ACTION == "vs2015"
        or _ACTION == "vs2017"
        or _ACTION == "vs2019"
        then

        VS_VERSION = _ACTION 

        if _ACTION == "vs2013" then
            VS_VERSION = 'vs13'
        elseif _ACTION == "vs2015" then 
            VS_VERSION = 'vs15'     
        end

        local action = premake.action.current()

        action.vstudio.windowsTargetPlatformVersion    = windowsPlatform
        action.vstudio.windowsTargetPlatformMinVersion = windowsPlatform

        if "vs2013-xp" == _OPTIONS["vs"] then
            premake.vstudio.toolset = ("v120_xp")
            -- location (path.join(_buildDir, "projects", _ACTION .. "-xp"))

        elseif "vs2015-xp" == _OPTIONS["vs"] then
            premake.vstudio.toolset = ("v140_xp")
            -- location (path.join(_buildDir, "projects", _ACTION .. "-xp"))

        elseif "vs2017-xp" == _OPTIONS["vs"] then
            premake.vstudio.toolset = ("v141_xp")
            -- location (path.join(_buildDir, "projects", _ACTION .. "-xp"))

        end
    elseif _ACTION and _ACTION:match("^xcode.+$") then

        local action = premake.action.current()
        local str_or = function(str, def)
            return #str > 0 and str or def
        end

        if "osx" == _OPTIONS["xcode"] then
            action.xcode.macOSTargetPlatformVersion = str_or(macosPlatform, "10.11")
            premake.xcode.toolset = "macosx"
            -- location (path.join(_buildDir, "projects", _ACTION .. "-osx"))

        elseif "ios" == _OPTIONS["xcode"] then
            action.xcode.iOSTargetPlatformVersion = str_or(iosPlatform, "8.0")
            premake.xcode.toolset = "iphoneos"
            -- location (path.join(_buildDir, "projects", _ACTION .. "-ios"))

        elseif "tvos" == _OPTIONS["xcode"] then
            action.xcode.tvOSTargetPlatformVersion = str_or(tvosPlatform, "9.0")
            premake.xcode.toolset = "appletvos"
            -- location (path.join(_buildDir, "projects", _ACTION .. "-tvos"))
        end
    end

end

if IS_XCODE then
    if _OPTIONS['xcode'] == 'ios' then
        IS_IOS = true
    else
        IS_MAC = true
    end
end