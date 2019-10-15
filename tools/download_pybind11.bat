@echo off
SET mydir=%~dp0
SET curdir=%cd%

CD %mydir%

REM TODO: add progress bar, -q is quite, if removing it the progress bar is in multiple lines
ECHO Downloading pybind
curl -sL --output pybind11_2_3_0.tar.gz https://github.com/pybind/pybind11/archive/v2.3.0.tar.gz

ECHO Unzipping pybind
tar xzf pybind11_2_3_0.tar.gz >NUL

CD %curdir%
