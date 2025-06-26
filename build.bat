@echo OFF
setlocal EnableDelayedExpansion

set script_dir_backslash=%~dp0
set script_dir=%script_dir_backslash:~0,-1%

if not exist Build mkdir Build
pushd Build
    REM Flags ======================================================================================
    REM MT   Static CRT
    REM EHa- Disable exception handling
    REM GR-  Disable C RTTI
    REM O2   Optimisation Level 2
    REM Oi   Use CPU Intrinsics
    REM Z7   Combine multi-debug files to one debug file
    set common_flags=-D DN_UNIT_TESTS_WITH_KECCAK %script_dir%\Source\Extra\dn_tests_main.cpp

    set msvc_driver_flags=%common_flags%        -MT -EHa -GR- -Od -Oi -Z7 -wd4201 -W4 -nologo

    REM Optionally pass `-analyze` to `msvc_compile_flags` for more checks, but,
    REM it slows down compilation by around 5s on my old laptop.
    set msvc_compile_flags=%msvc_driver_flags%  -analyze -fsanitize=address /Fe:dn_unit_tests_msvc
    set clang_compile_flags=%msvc_driver_flags% -fsanitize=address -fsanitize=undefined /Fe:dn_unit_tests_clang
    set zig_compile_flags=%common_flags%        -fsanitize=address -fsanitize=undefined -o dn_unit_tests_zig

    set msvc_link_flags=-link
    set clang_link_flags=%msvc_link_flags%

    REM msvc =======================================================================================
    set has_msvc=1
    where /q cl || set has_msvc=0
    if %has_msvc% == 1 (
        echo [BUILD] MSVC's cl detected, compiling ...
        set msvc_cmd=cl %msvc_compile_flags% %msvc_link_flags%
        powershell -Command "$time = Measure-Command { !msvc_cmd! | Out-Default }; Write-Host '[BUILD] msvc:'$time.TotalSeconds's'; exit $LASTEXITCODE" || exit /b 1

        call cl %script_dir%\single_header_generator.cpp -nologo -link
        call single_header_generator.exe %script_dir%\Source %script_dir%\Single_Header
    )


    REM REM clang-cl ===================================================================================
    set has_clang_cl=1
    where /q clang-cl || set has_clang_cl=0
    if %has_clang_cl% == 1 (
        echo [BUILD] clang-cl detected, compiling ...
        set clang_cmd=clang-cl %clang_compile_flags% %clang_link_flags%
        powershell -Command "$time = Measure-Command { !clang_cmd! | Out-Default }; Write-Host '[BUILD] clang-cl:'$time.TotalSeconds's'; exit $LASTEXITCODE" || exit /b 1
    )

    REM zig ========================================================================================
    REM TODO(doyle):Can't build "Misc\dn_unit_tests.cpp|1 col 1| error: unable to build C object: FileNotFound"
    REM set has_zig=1
    REM where /q zig || set has_zig=0
    REM if %has_zig% == 1 (
    REM     echo [BUILD] zig detected, compiling ...
    REM     set zig_cmd=zig c++ %zig_compile_flags%
    REM     powershell -Command "$time = Measure-Command { !zig_cmd! | Out-Default }; Write-Host '[BUILD] zig:'$time.TotalSeconds's'; exit $LASTEXITCODE" || exit /b 1
    REM )

    exit /b 1
popd
