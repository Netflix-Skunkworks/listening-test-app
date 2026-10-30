@echo off
rem Copyright(C) 2026 Netflix, Inc.
setlocal EnableDelayedExpansion
cd /d "%~dp0"

git submodule update --init --recursive
if errorlevel 1 goto :failed
for /f "delims=" %%a in ('git config --file .gitmodules submodule.JUCE.tag') do set "JUCE_TAG=%%a"
if not defined JUCE_TAG goto :failed
set "JUCE_HEAD="
set "JUCE_TAG_COMMIT="
for /f "delims=" %%a in ('git -C JUCE rev-parse HEAD') do set "JUCE_HEAD=%%a"
rem A temporary file avoids FOR /F's second parsing of the tag's caret.
set "JUCE_CHECK_FILE=%TEMP%\listening-test-juce-%RANDOM%-%RANDOM%.txt"
git -C JUCE rev-parse "%JUCE_TAG%^{commit}" > "%JUCE_CHECK_FILE%"
if errorlevel 1 goto :failed
set /p JUCE_TAG_COMMIT=<"%JUCE_CHECK_FILE%"
del "%JUCE_CHECK_FILE%"
if not defined JUCE_HEAD goto :failed
if not "%JUCE_HEAD%"=="%JUCE_TAG_COMMIT%" (
    >&2 echo error: JUCE submodule must be at tag %JUCE_TAG%
    goto :failed
)

rem Respect an explicit MSBUILD, otherwise use the developer prompt or vswhere.
rem Quote vswhere wildcards. An unquoted * is expanded to files in this repo
rem before vswhere runs, so Build Tools are not found.
if defined MSBUILD goto :msbuild_ready
where msbuild >nul 2>nul
if not errorlevel 1 (
    for /f "delims=" %%a in ('where msbuild') do if not defined MSBUILD set "MSBUILD=%%a"
    goto :msbuild_ready
)
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
if not exist "!VSWHERE!" goto :failed
rem A temporary file avoids FOR /F splitting the quoted vswhere path on spaces.
set "VSINSTALL_FILE=%TEMP%\listening-test-vsinstall-%RANDOM%-%RANDOM%.txt"
"%VSWHERE%" -latest -products "*" -requires Microsoft.Component.MSBuild -property installationPath > "%VSINSTALL_FILE%"
if errorlevel 1 goto :failed
set "VSINSTALL="
set /p VSINSTALL=<"%VSINSTALL_FILE%"
del "%VSINSTALL_FILE%"
if not defined VSINSTALL goto :failed
set "MSBUILD=%VSINSTALL%\MSBuild\Current\Bin\MSBuild.exe"
if not exist "!MSBUILD!" (
    >&2 echo error: MSBuild was not found at !MSBUILD!.
    goto :failed
)
:msbuild_ready
rem Prefer an explicit TOOLSET. Otherwise use the newest toolset installed
rem beside this MSBuild: v145 (Build Tools 2026), v143 (VS2022), v142 (VS2019).
if not defined TOOLSET call :detect_toolset
if not defined TOOLSET (
    >&2 echo error: no v145, v143, or v142 toolset was found beside !MSBUILD!.
    goto :failed
)
echo Platform toolset: %TOOLSET%
if not defined BUILD_CONFIGURATION set "BUILD_CONFIGURATION=Debug"
if not "%BUILD_CONFIGURATION%"=="Debug" if not "%BUILD_CONFIGURATION%"=="Release" goto :failed

rem JUCE 6.1.6 ships a VS2019 project. Override the toolset for VS2022/2026.
"%MSBUILD%" JUCE\extras\Projucer\Builds\VisualStudio2019\Projucer.sln /m /p:Configuration=Debug /p:Platform=x64 /p:PlatformToolset=%TOOLSET%
if errorlevel 1 goto :failed
JUCE\extras\Projucer\Builds\VisualStudio2019\x64\Debug\App\Projucer.exe --resave listening-test.jucer
if errorlevel 1 goto :failed
"%MSBUILD%" "Builds\VisualStudio2019\Listening Test.sln" /m /p:Configuration=%BUILD_CONFIGURATION% /p:Platform=x64 /p:PlatformToolset=%TOOLSET%
if errorlevel 1 goto :failed
echo Application: %CD%\Builds\VisualStudio2019\x64\%BUILD_CONFIGURATION%\App\Listening Test.exe
exit /b 0
:failed
>&2 echo error: Windows build failed. Install C++ desktop build tools and a Windows SDK; see README.md.
exit /b 1

:detect_toolset
set "VC_MSBUILD="
for %%I in ("!MSBUILD!") do set "VC_MSBUILD=%%~dpI..\..\Microsoft\VC"
for %%T in (v145 v143 v142) do (
    if not defined TOOLSET (
        if exist "!VC_MSBUILD!\v180\Platforms\x64\PlatformToolsets\%%T\Toolset.props" set "TOOLSET=%%T"
        if exist "!VC_MSBUILD!\v170\Platforms\x64\PlatformToolsets\%%T\Toolset.props" set "TOOLSET=%%T"
        if exist "!VC_MSBUILD!\v160\Platforms\x64\PlatformToolsets\%%T\Toolset.props" set "TOOLSET=%%T"
    )
)
exit /b 0
