@echo OFF
setlocal EnableDelayedExpansion

set script_dir_backslash=%~dp0
set script_dir=%script_dir_backslash:~0,-1%
set build_dir=%script_dir%\Build

if not exist %build_dir% mkdir %build_dir%
pushd %build_dir%

 :: Build CURL if we can and incorporate it into the compile flags
  where /q cmake && (
    if not exist %build_dir%/Curl/Install/lib/libcurl-d.lib (
      cmake -B %build_dir%/Curl ^
            -S %script_dir%/External/curl-8.17.0 ^
            -G Ninja ^
            -D BUILD_SHARED_LIBS=OFF ^
            -D BUILD_STATIC_LIBS=ON ^
            -D BUILD_CURL_EXE=OFF ^
            -D BUILD_LIBCURL_DOCS=OFF ^
            -D BUILD_MISC_DOCS=OFF ^
            -D ENABLE_CURL_MANUAL=OFF ^
            -D CURL_USE_SCHANNEL=ON ^
            -D CURL_USE_LIBPSL=OFF ^
            -D CURL_STATIC_CRT=ON ^
            -D CMAKE_INSTALL_PREFIX=%build_dir%/Curl/Install

      cmake --build %build_dir%/Curl --parallel --target install
    )
  )

  :: MT   Static CRT
  :: EHa- Disable exception handling
  :: GR-  Disable C RTTI
  :: Oi   Use CPU Intrinsics
  :: Z7   Combine multi-debug files to one debug file
  set flags=%flags% -D DN_UNIT_TESTS_WITH_KECCAK %script_dir%\Source\Extra\dn_tests_main.cpp
  set msvc_driver_flags=-EHa -GR- -Od -Oi -Z7 -wd4201 -W4 -nologo %flags% -fsanitize=address

  where /q emcc && (
    echo [BUILD] Emscripten emcc detected, compiling ...
    call emcc -g -msimd128 -msse2 %flags% -o %build_dir%\dn_unit_tests_emcc.js -s FETCH=1 -pthread -s ASYNCIFY=1 -lwebsocket -Wall
  )

  where /q cl && (
    echo [BUILD] MSVC cl detected, compiling ...
    set msvc_cmd=cl -MTd %msvc_driver_flags% -analyze -Fe:dn_unit_tests_msvc -Fo:dn_unit_tests_msvc
    if exist %build_dir%/Curl/Install/lib/libcurl-d.lib (
      set msvc_cmd=!msvc_cmd! -D DN_UNIT_TESTS_WITH_CURL -I %build_dir%/Curl/Install/include %build_dir%/Curl/Install/lib/libcurl-d.lib crypt32.lib ws2_32.lib advapi32.lib wldap32.lib iphlpapi.lib secur32.lib
    )
    set msvc_cmd=!msvc_cmd! -link

    powershell -Command "$time = Measure-Command { !msvc_cmd! | Out-Default }; Write-Host '[BUILD] msvc:'$time.TotalSeconds's'; exit $LASTEXITCODE" || exit /b 1
    call cl %script_dir%\single_header_generator.cpp -Z7 -nologo -link
    call %build_dir%\single_header_generator.exe %script_dir%\Source %script_dir%\Single-Header
  )

  where /q clang-cl && (
    echo [BUILD] LLVM clang-cl detected, compiling ...
    set clang_cmd=clang-cl -MT %msvc_driver_flags% -fsanitize=undefined -Fe:dn_unit_tests_clang -link
    powershell -Command "$time = Measure-Command { !clang_cmd! | Out-Default }; Write-Host '[BUILD] clang-cl:'$time.TotalSeconds's'; exit $LASTEXITCODE" || exit /b 1
  )

  exit /b 1
popd
