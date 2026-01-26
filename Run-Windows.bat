set solution_directory=.\solution

if not exist %solution_directory% mkdir %solution_directory%

cd %solution_directory%
del CMakeCache.txt
cmake .. -DVKE_RENDER_SYSTEM_VULKAN=ON

if %ERRORLEVEL% equ 0 start vkEngine.sln
