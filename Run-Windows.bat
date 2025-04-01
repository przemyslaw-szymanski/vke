set solution_directory=.\solution

if not exist %solution_directory% mkdir %solution_directory%

cd %solution_directory%
cmake ..

if %ERRORLEVEL% equ 0 start vkEngine.sln
