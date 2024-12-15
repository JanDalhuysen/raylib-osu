@REM Windows steps to compile a raylib game using MinGW

@REM Create a new conda environment

conda create --name test1 python=3.8

@REM Activate the new environment

conda activate test1

@REM Install git, gcc, and cmake

conda install conda-forge::git

conda install conda-forge::gcc

conda install conda-forge::cmake

@REM Generate build files using cmake

cmake . -G "MinGW Makefiles"

@REM Compile the game using mingw32-make

mingw32-make.exe

@REM Run the game

main.exe
