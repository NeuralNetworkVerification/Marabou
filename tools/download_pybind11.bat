@echo off
SET mydir=%~dp0
SET curdir=%cd%

CD %mydir%
ECHO %mydir%

REM TODO: add progress bar, -q is quite, if removing it the progress bar is in multiple lines
ECHO Downloading pybind
wget -q -O pybind11_2_3_0.tar.gz https://github.com/pybind/pybind11/archive/v2.3.0.tar.gz

ECHO Unzipping pybind
tar xzvf pybind11_2_3_0.tar.gz >NUL

CD %curdir%