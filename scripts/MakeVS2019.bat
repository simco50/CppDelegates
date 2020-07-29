echo off
cls

echo Building for Visual Studio 2019...
"%~dp0premake5" vs2019 --file="%~dp0premake5.lua"