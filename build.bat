@echo off
setlocal

git submodule init && git submodule update
if errorlevel 1 exit /b 1

rem Refuse to build against a JUCE the licence does not cover. Git pins a
rem submodule by commit rather than by tag, so the tag is named in
rem .gitmodules and checked against what is actually checked out.
for /f "delims=" %%a in ('git config --file .gitmodules submodule.JUCE.tag') do set "JUCE_TAG=%%a"
set "JUCE_HEAD="
set "JUCE_TAG_COMMIT="
for /f "delims=" %%a in ('git -C JUCE rev-parse HEAD') do set "JUCE_HEAD=%%a"
rem Peel the tag outside FOR /F. A FOR /F command is parsed a second time,
rem and that second parse turns "^{commit}" into "{commit}", so rev-parse
rem fails and the check reports a mismatch even when HEAD is the tag.
git -C JUCE rev-parse "%JUCE_TAG%^{commit}" > "%TEMP%\juce-tag-commit.txt"
if errorlevel 1 (
    >&2 echo error: cannot resolve JUCE tag %JUCE_TAG%
    >&2 echo        run: git -C JUCE fetch --tags ^&^& git -C JUCE checkout %JUCE_TAG%
    exit /b 1
)
set /p JUCE_TAG_COMMIT=<"%TEMP%\juce-tag-commit.txt"
if not "%JUCE_HEAD%"=="%JUCE_TAG_COMMIT%" (
    >&2 echo error: JUCE submodule is not at tag %JUCE_TAG%
    >&2 echo        run: git -C JUCE fetch --tags ^&^& git -C JUCE checkout %JUCE_TAG%
    exit /b 1
)

set "MSBUILD=C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\MSBuild\Current\Bin\MSBuild.exe"
rem Build Tools 2026 ships the v145 toolset. The JUCE projects were
rem generated for v143 (VS2022) and v142 (VS2019), which are not installed.
set "TOOLSET=v145"

rem Projucer is built from the newest Visual Studio project shipped in the
rem JUCE submodule, then run to generate the app's VS2019 project from
rem listening-test.jucer.
"%MSBUILD%" JUCE\extras\Projucer\Builds\VisualStudio2022\Projucer.sln /p:Configuration=Debug /p:Platform=x64 /p:PlatformToolset=%TOOLSET%
if errorlevel 1 exit /b 1

JUCE\extras\Projucer\Builds\VisualStudio2022\x64\Debug\App\Projucer.exe --resave listening-test.jucer
if errorlevel 1 exit /b 1

"%MSBUILD%" "Builds\VisualStudio2019\Listening Test.sln" /p:Configuration=Debug /p:Platform=x64 /p:PlatformToolset=%TOOLSET%
if errorlevel 1 exit /b 1
