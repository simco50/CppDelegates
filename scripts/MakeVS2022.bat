echo off
cls

echo Building for Visual Studio 2022...
"%~dp0premake5" vs2022 --file="%~dp0premake5.lua"