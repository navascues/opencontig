@echo off
pushd "%~dp0"

del *.ncb /S /Q
del *.sdf /S /Q
del *.suo /S /Q /F /A:H
del *.user /S /Q

rmdir Debug /S /Q
rmdir Release /S /Q

popd