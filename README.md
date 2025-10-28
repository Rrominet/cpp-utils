# cpp-utils

It's a collection of usefull functions and classes for c++ projects  
All the cpp projects I've done so far are dependent on it.

> [!NOTE]
> This library has only been tested on linux for now.  
> All the following instructions are for linux (particulary debian based system, but should work on all linuxes)

# Installation

This repo need to be build as a shared library (.so)
> [!NOTE]
> In a near future, you'll be able to grab the prebuild binaries.  
> But for now, you'll have to build it yourself.  
> You'll see it's pretty straightforward, I made you a script that do all for you. *(I know, I know, I'm too good)*

So, to build it yourself, nothing easier.  
`git clone` this repo and execute the file `build-n-install` as `sudo`.

That's it.  
I told you it was pretty straightforward.

For your convinience here the code that you can copy and past in a terminal directly :
```bash
git clone https://github.com/Rrominet/cpp-utils.git
cd cpp-utils
sudo chmod +x ./build-n-install
sudo ./build-n-install
```

> [!WARNING]
> In the `build-n-install` script, it install some dependencies with `apt`.  
> BUT if you are on a non-debian system, it won't work.  
> In that case, just adabt the first line of the script `sudo apt install ...` to your system's package manager.  
> FYI, here are the libs installed by the script :   
> `build-essential` `git` `gcc` `g++` `wget` `zip` `unzip` `libboost-all-dev`

After the installtion done, you'll see a new directory called `cpp-utils_dependencies` in the parent directory of the `cpp-utils` local repo.

If you just want to use the `libmlapi` (cpp-utils) library as a shared lib, you can remove the `cpp-utils_dependencies`.
But if you want to rebuild it or change the code, or use it as a lib in one of your own project, you need to keep it.  
*(It contains the headers of the dependencies needed for the compilation process.)*

# Usage

For now, it's usage is basicly a dependency for the other cpp projects I've done so far.  
The projects dependent on it will basicly clone this repo and do the installation directly.

You also can use it as a shared lib on your own project but I need to write the documentation for that.  
Waiting for this to happen, you can always check the content of the [make](https://github.com/Rrominet/cpp-utils/blob/main/make) file to see the different `includes` and libraries links you need to do.
The `make` script is a file that run `fxmake`, my own builder that you can find in [this repo](https://github.com/Rrominet/py-utils/tree/main/build).

# TODO

Well, a lot! Like a doc and a process to link the lib in your custom projects.  
Also a release system so you can grab the binaries instead of building it yourself.
