To setup a out of tree build for clang plugin
1. Copy the directory cmake and the file CMakeLists.txt as it is
2. Copy your plugin into a new sub_directory
3. Update CMakeLists.txt to add your sub_directory (last line)
4. To build either run the build.sh script or run the following command

    a) mkdir .build && cd .build

    b) cmake ..
  
    c) make

    d) cd ..
