# CSE231-Advanced-Compilers

Programming assignment of CSE 231 at UCSD (Winter 2020) [[link](https://ucsd-pl.github.io/cse231/wi20/project.html)]

```bash
├── Guides
├── Output
│   ├── testcases
├── Passes
│   ├── DFA
│   │   ├── 231DFA.h
│   │   ├── LivenessAnalysis.cpp
│   │   ├── MayPointToAnalysis.cpp
│   │   ├── ReachingDefinitionAnalysis.cpp
│   │   └── CMakeLists.txt
│   ├── part1
│   │   ├── BranchBias.cpp
│   │   ├── CountDynamicInstructions.cpp
│   │   ├── CountStaticInstructions.cpp
│   │   └── CMakeLists.txt
│   ├── testPass
│   │   ├── TestPass.cpp
│   │   └── CMakeLists.txt
│   └── CMakeLists.txt
```

## Table of Content
* [Set Up the Environment](https://github.com/wwqqqqq/CSE231-Advanced-Compilers/blob/master/README.md#set-up-the-environment)
  * [Install Docker](https://github.com/wwqqqqq/CSE231-Advanced-Compilers/blob/master/README.md#install-docker)
  * [Pull the LLVM image](https://github.com/wwqqqqq/CSE231-Advanced-Compilers/blob/master/README.md#pull-the-llvm-image-for-cse-231-course-project-only)
  * [Start a shell in the LLVM docker image](https://github.com/wwqqqqq/CSE231-Advanced-Compilers/blob/master/README.md#start-a-shell-in-the-llvm-docker-image)
* [How to compile LLVM Pass](https://github.com/wwqqqqq/CSE231-Advanced-Compilers/blob/master/README.md#how-to-compile-llvm-pass)
* [How to Generate LLVM IR Code from Source Code](https://github.com/wwqqqqq/CSE231-Advanced-Compilers/blob/master/README.md#how-to-generate-llvm-ir-code-from-source-code)
  * [How to Write Test Cases that can Generate LLVM IR with PHI Instructions](https://github.com/wwqqqqq/CSE231-Advanced-Compilers/blob/master/README.md#how-to-write-test-cases-that-can-generate-llvm-ir-with-phi-instructions)
* [How to Run LLVM Pass](https://github.com/wwqqqqq/CSE231-Advanced-Compilers/blob/master/README.md#how-to-run-llvm-pass)
* [Reference](https://github.com/wwqqqqq/CSE231-Advanced-Compilers/blob/master/README.md#reference)


## Set Up the Environment
### Install [Docker](https://www.docker.com/get-started)
### Pull the LLVM image (for CSE 231 Course Project only)
``` bash
$ docker pull yalhessi/cse231_student:llvm9
(Docker should automaticaly start downloading and extracting the provided LLVM image.)

(To verify the completion of the last step, run the following command)
$ docker images
```
### Start a shell in the LLVM docker image
  - Linux: 
``` bash
$ sudo ./mount_and_launch.sh
```
  - Windows: Some windows machines won't allow execution of scripts in Powershell. 
You will have to edit the paths in the following command to match your configuration and copy-paste it into a powershell to start the provided image. 
``` 
$ docker run --rm -it -v <Path to Output Directory in local machine>:/output -v <Path to Passes directory in local machine>:/LLVM_ROOT/llvm/lib/Transforms/CSE231_Project yalhessi/cse231_student:llvm9 /bin/bash
```

Note:
1. The option `--rm` will automatically kill the docker image as soon as you exit it so it doesn't run in the background.
2. The option `-it` will open an interactive session of the image.
3. The `-v` options mount the specified folders to the mount points (path-on-host:mount-point-in-docker).

Reference: [Docker run reference](https://docs.docker.com/storage/bind-mounts/), [Use bind mounts](https://docs.docker.com/storage/bind-mounts/)

## How to compile LLVM Pass
 - Start the provided docker image by running `sudo ./mount_and_launch.sh`
 - Your `src` folder should be mounted at `/LLVM_ROOT/llvm/lib/Transforms/CSE231_Project`

``` bash
$ cd /LLVM_ROOT/build
$ cmake /LLVM_ROOT/llvm
(... cmake ends without errors ...)
$ cd /LLVM_ROOT/build/lib/Transforms/CSE231_Project
$ make
(Pass can now be found under /LLVM_ROOT/build/lib)
```

If everything went well you should be able to find your module (an LLVM module includes passes) under `/LLVM_ROOT/build/lib`.

Reference: [Cmake vs make sample codes](https://stackoverflow.com/questions/10882030/cmake-vs-make-sample-codes), [CMake vs Make](https://prateekvjoshi.com/2014/02/01/cmake-vs-make/)

## How to Generate LLVM IR Code from Source Code
 - Start the provided docker image by running `sudo ./mount_and_launch.sh`
 - Your `tests` folder should be mounted at `/tests` within the container. Verify.
 - Move into the tests folder: `cd /tests`
 - cd into the folder with the test case you want to compile. For example: `cd HelloWorld`
 - Compile IR code: `clang -S -emit-llvm HelloWorld.cpp`
 - Done! You should be able to see a `HelloWorld.ll` file. 
 - The file should be immediately available on your local (host) machine as well. You can exit the docker image without losing it.
 
### How to Write Test Cases that can Generate LLVM IR with PHI Instructions
1. To have a single phi node in your code, run the following command:
``` bash
$ clang -O0 -S -emit-llvm test.cpp
```

`test.cpp` can be:
``` C
int test(){
        int x,y,z;
        z = x || y;
        z = x && y;
        bool b;
        z = b? x :y;
        return z;
}
```
2. To generate multiple phi nodes inside a block, the commands are:
``` bash
$ clang -S -emit-llvm -O -Xclang -disable-llvm-passes test.cpp -o test.ll
$ opt -mem2reg -S test.ll -o test_phinodes.ll
```
In LLVM a phi node can have more than two inputs, so even when a switch has multiple branches, 
if you only use it to modify one variable, there's only going to be one phi node. 
To generate multiple phi nodes, multiple variables need to be modified in different branches. 
`clang` also seems to generate loads and stores instead of llvm registers and phi nodes when compiled with `-O0`, 
so you may want to use `-O1`. 
As an example, the following code generates two consecutive phi nodes after the conditional for y and z respectively when compiled with `-O1` optimization level.

```C
int compute(){
        int x, m, n;
        int y,z;
        y = m;
        z = n;
        if (x > 0){
                y = n * m;
                z = n + m;
        }
        return y - z;
}
```

Reference: [stackoverflow](https://stackoverflow.com/questions/46513801/llvm-opt-mem2reg-has-no-effect), [piazza@130](https://piazza.com/class/k4yjt67yqmj7dn?cid=130), [piazza@233](https://piazza.com/class/k4yjt67yqmj7dn?cid=233)

## How to Run LLVM Pass
 - Start the provided docker image by running `sudo ./mount_and_launch.sh`
 - Follow the steps in `HOW_TO_COMPILE_LLVM_PASS.txt`. DO NOT exit the running container.
 - The environment is pre-configured so you can run the pass from any directory in the filesystem. 
 - To run your pass: `opt -load LLVMTestPass.so -TestPass < /tests/HelloWorld/HelloWorld.ll > /dev/null`
 - Our pass sends output to the `errs()` device (standard error). We can redirect the output by appending `2> <file-path>` in the previous command. *e.g.* `opt -load LLVMTestPass.so -TestPass < /tests/HelloWorld/HelloWorld.ll > /dev/null 2> /output/test_pass_output.txt`
 - Done!
 
Note:
1. `opt` is LLVM's command line tool for executing passes.
2. `-load LLVMTestPass.so` causes `opt` to load the shared library that contains the TestPass. We don't need to specify the full path to the pass since the docker image's environment was appropriately configured to know where to search. The name of the module (`LLVMTestPass`) is defined in `CMakeLists.txt`.
3. `-TestPass` tells `opt` the name of the pass we wish to run. The pass was bound to the `-TestPass` command line flag by the [`RegisterPass<TestPass>`](https://github.com/wwqqqqq/CSE231-Advanced-Compilers/blob/e5a31878d7bf0590dd7118e5432ecd183a37e713/Passes/testPass/TestPass.cpp#L21) declaration in [`TestPass.cpp`](https://github.com/wwqqqqq/CSE231-Advanced-Compilers/blob/master/Passes/testPass/TestPass.cpp).
4. By default, the output of `opt` is a transformed program. Since the Hello pass doesn't perform any transformations, we just redirect the output to `/dev/null` to ignore it.
 
## Reference
1. [LLVM](http://llvm.org/docs/) Documentations:
    - [Writing An LLVM Pass](http://llvm.org/docs/WritingAnLLVMPass.html)
    - [LLVM Programmer's Manual](http://llvm.org/docs/ProgrammersManual.html)
    - [Helpful Hints for Common Operations](http://llvm.org/docs/ProgrammersManual.html)
    - [LLVM::Instruction Class Reference](https://llvm.org/doxygen/classllvm_1_1Instruction.html) 
    - [LLVM::BasicBlock Class Reference]( https://llvm.org/doxygen/classllvm_1_1BasicBlock.html)
    - [LLVM::Function Class Reference](https://llvm.org/doxygen/classllvm_1_1Function.html)
    - [LLVM::Pass Class Reference](https://llvm.org/doxygen/classllvm_1_1Pass.html)
    - [LLVM::IRBuilder< T, Inserter > Class Template Reference](https://llvm.org/doxygen/classllvm_1_1IRBuilder.html)
2. How to Write CMakeLists.txt
    - [CMake Tutorial](https://cmake.org/cmake/help/latest/guide/tutorial/index.html)
    - [Learning CMake a Beginners' Guide](https://tuannguyen68.gitbooks.io/learning-cmake-a-beginner-s-guide/content/)
 
