The `cpp-utils` lib is a collection of `class` and `functions` that will clearly make your life easier when coding in C++.
Its goal is to help you do anything except the GUI relative stuff. (for this, check out `fxos_gui-lib`)

Like : 

 - Multithreading
 - Multiprocessing
 - Filesystem wrappers
 - Network (client and server)
 - IPC
 - String manipulation
 - Time manipulation
 - and (a lot) more

## Installation

> [!NOTE]
> This library has only been tested on linux for now.  
> All the following instructions are for linux (particulary debian based system, but should work on all linuxes)

This lib need to be build as a shared library (.so)

> [!NOTE]
> In a near future, you'll be able to grab the prebuild binaries.  
> But for now, you'll have to build it yourself.  
> You'll see it's pretty straightforward, I made you a script that do all for you. *(I know, I know, I'm too good)*

So, to build it yourself, nothing easier.  
`git clone` this [repo](https://github.com/Rrominet/cpp-utils.git) and execute the file `build-n-install` as `sudo`.

That's it.  
I told you it was pretty straightforward.

For your convinience here the code that you can copy and past in a terminal directly :
```bash
git clone https://github.com/Rrominet/cpp-utils.git
cd cpp-utils
sudo chmod +x ./build-n-install
sudo ./build-n-install
```

> [!NOTE]
> The script should install all the packages needed for the compilation process for any major distros.  
> If you have an error, install the packages manually.
> FYI, here are the libs installed by the script :   
> `build-essential` `git` `gcc` `g++` `wget` `zip` `unzip` `libboost-all-dev`

After the installtion done, you'll see a new directory called `cpp-utils_dependencies` in the parent directory of the `cpp-utils` local repo.

This folder contains all the headers needed to be included in your differrent projects that would use `cpp-utils`.

## How to import it in your project ?

So be able to use `cpp-utils`, you need 2 things : 

 - The header files (.h) included at build time.
 - The shared lib (.so) linked at link time.

### Including the header files

Stupidly simple : 
Add the `cpp-utils_dependencies` folder fullpath in to your include dirs at build time.

If you call your builder directly, here is the example with `g++` (building here a program that has one file to compile : `main.cpp`) :

```bash
g++ -I/path/to/cpp-utils_dependencies/ -I/other/include/dir -c main.cpp
```

`clang` is the same : 

```bash
clang++ -I/path/to/cpp-utils_dependencies/ -I/other/include/dir -c main.cpp
```

If you use `cmake` just add this line before the `add_executable` line :

```cmake
include_directories(/path/to/cpp-utils_dependencies/)
```

If you use `meson` here is the python code : 
```python
inc = include_directories('/path/to/cpp-utils_dependencies/', '/other/include/dir')
executable('app', 'main.cpp', include_directories: inc)
```

If you use `fxmake` (our build tool - check it out [here](https://github.com/Rrominet/py-utils/tree/main/build))
Here is the python code :

```python
prj = build.create("executable_name")
prj.includes = ["/path/to/cpp-utils_dependencies/", "/other/include/dir"]
```

### Linking the shared lib

Now that you included the headers, it's time to link the shared lib.
The shared lib is done after the compile part, at link time.

Meaning that under the hood, your builder (in the example `g++`) will first call on all your cpp file : 
```bash
g++ -I/path/to/cpp-utils_dependencies/ -c one-of-your-src.cpp
```

And after all your files compile to a `.o` file obect, you link them together to create your final executable : 
```bash
g++ file-1.o file-2.o ... -L/path/to/cpp-utils_dependencies/ -lmlapi -o app
```
The dir under `-L` is the directory where is your compiled shared lib (`libmlapi.so`).
The filename under `-l` is `libmlapi.so` without the extension and without the prefix `lib`.

You can actually also directly link the shared lib file fullpath with the `-l` like this : 
```bash
g++ file-1.o file-2.o ... /path/to/cpp-utils_dependencies/libmlapi.so -o app
```

This is if you do it by hand but of course, but you certainly use `cmake`, `meson` or equivalent.

So, here is the `cmake` code :

```cmake
add_executable(app your_cpp_files)
target_link_directories(app PRIVATE /path/to/cpp-utils_dependencies)
target_link_libraries(app PRIVATE mlapi)
```

Or with the fullpath : 
```cmake
target_link_libraries(app PRIVATE /path/to/cpp-utils_dependencies/libmlapi.so)
```

Here is the `meson` python code : 
```python
foo = cc.find_library('foo', dirs: '/path/to/cpp-utils_dependencies')
executable('app', 'main.cpp', dependencies: mlapi)
```

And if you use `fxmake`: 
```python
prj = build.create("executable_name")
prj.addToLibDirs("/path/to/cpp-utils_dependencies/")
prj.addToLibs("mlapi")

# or directly the full library path : 
prj.addToLibs("/path/to/cpp-utils_dependencies/libmlapi.so")
```

With the included headers and the shared lib linked, your program should build with no problem.
But not sure you can run it yet...

### How to correct the *Runtime missing Library* error (`rpath`)

So, first of all, know that if you use `fxmake`, you shouldn't have this problem because the builder does it for automaticly.
Else, if you have a missing shared lib error when you try to launch your program build against `-cpp-utils` (file `libmlapi.so`), it's for you.

The reason of this problem is deadly simple : 
Linux don't find the file `libmlapi.so` when launching your program.

Certainly because your `libmlapi.so` is not in the default dirs where linux search (`/usr/lib` and similar).

So you basicly have 2 options here :

 - You simply copy the `libmlapi.so` file in `/usr/lib` (or prefer `/usr/local/lib` to avoid managers managed program agains custon installations)
 - Or you tell linux where to find it using something called `rpath`.

#### How to set `rpath` at build time ?
`rpath` is simply a list of dirs where linux will look for your lib shared file (.so) when running your executable.
You can set it like this :

You simply need to add this folowing arguments to your link cmd : 
```bash
g++ file-1.o file-2.o ... -L/path/to/cpp-utils_dependencies/ -lmlapi -o app -Wl,-rpath,/the/dir/where/is/installed/libmlapi.so/on/your/system
```

With `cmake` : 
```cmake
set_target_properties(app PROPERTIES
    INSTALL_RPATH "/the/dir/where/is/installed/libmlapi.so/on/your/system"
)
```

With `meson`: 
```python
executable(
  'app',
  'main.cpp',
  install: true,
  install_rpath: '/the/dir/where/is/installed/libmlapi.so/on/your/system'
)
```

#### How to set `rpath` after the build ?

What if you forgot to add the rpath at build time ?
No problem, you can set it directly on the executable file without the need to recompile it.

For this, you will use a tool called `patchelf` ([find it here](https://github.com/NixOS/patchelf)):

Just do : 
```bash
patchelf --set-rpath /the/dir/where/is/installed/libmlapi.so/on/your/system your-compiled-app
```

## How to use it ?

Ok now that you can build and run a program agains `cpp-utils`, here how to use it.

The API propose basicly 2 interfaces you can call : 

 - Functions under a namespace
 - Class you can instanciate

### Functions under a namespace : 

For example, you have the file `storage.h` wich give you access to all the functions under the namepace `storage`.
Here what a `.cpp` file would look like if you would want to use it :
```cpp
#include "storage.h"
...

storage::init();
storage::set<std::string>("my-data", "My Awesome Data");

...
```

>[!note]
>The *storage* module that let you save data on disk from the app name without woriing about the files and other stuff.
>Under the hood, it write the data in `json` format in `~/.config/<app-name>/storage.json`

### Class you can instanciate :

For example, you could instanciate the class `ThreadPool` in the file `thread.h`.
To do this is deadly simple: 

```cpp
#include "thread.h"

---

//note : the ThreadPool class is under the namespace `th`
th::ThreadPool pool();

...

pool.run(whatever_you_want_to_run);
...
```

## Most useful modules and examples.

The `cpp-utils` lib is quite large and it can be a bit tricky to navigate.
So, here are the modules that I personnaly use the most and how to use them :

### The filesystem related modules

It's a list of modules (under the path `/files.2`) that abstract a lot of the filesystem related stuff that will make your life with files and dirs 100 times easier.

First of all, the `files.h` module that gives you access to the `files` namespace.
Simple examples : 
```cpp
#include <vector>
#include <string>

#include "files.2/files.h"
...
//takes care of the fd, open and closed automaticly
files::write("/path/to/file", "Whatever you want to write");
// or in appending mode 
files::append("/path/to/file", "Whatever you want to write");

//to read files
std::string data = files::read("/path/to/file");

//or in binary mode
std::vector<unsigned char> data = files::bContent("/path/to/file");

//get files in a dir
std::vector<std::string> files = files::ls("/path/to/dir");
// or reccursively
std::vector<std::string> files = files::ls_reccursive("/path/to/dir");
```
`Ctrl F` -> `files` to access all of the functions in the `files` namespace.

### The module `str` (file : `str.h`)

`str.h` is a list of functions to manipulate strings.
Here are some examples : 

```cpp
#include "str.h"

std::vector<std::string> words = str::split("This is a sentence", " ");
std::string joined = str::join(words, " ");
std::string lower = str::lower("This is a sentence"); //will give "this is a sentence"
std::string upper = str::lower("This is a sentence"); //will give "THIS IS A SENTENCE"
bool res = str::contains("This is a sentence", "is"); //will give true
char last = str::last("This is a sentence"); //will give 'e'
```

It's basic stuff (there some advanced stuff too) but it's not in the cpp `stdlib` so... Now you have it.

`Ctrl F` -> `str` to access all of the functions in the `str` namespace.

### The module `vec` (file : `vec.h`)

`vec.h` is a list of function to manipulate `std::vector` or equivalent.
The functions are under the `vc` namespace

Here are some examples : 

```cpp
#include "vec.h"

std::vector<int> myVector{2, 34, 42, 77};
int removed = vc::remove(myVector, 42);

double average = vc::average(myVector);
bool res = vc::contains(myVector, 42); //here would return false because 42 is not in the vector anymore after the vc::remove call
```

But actually, the most useful stuff in the `vec.h` module is the class `Vec` under the namespace `ml` : 

```cpp
#include "vec.h"

ml::Vec<int> myVector{2, 34, 42, 77};

int removed = myVector.remove(42);

double average = myVector.average();
bool res = myVector.contains(42); //here would return false because 42 is not in the vector anymore after the ml::Vec::remove() call
```

`Ctrl F` -> `vec` to access all of the functions in the `vc` namespace.


### The module `random`

>[!note]
>This part still need to be written.


### The module `thread`

>[!note]
>This part still need to be written.


### The module `mlprocess`

>[!note]
>This part still need to be written.


### The modules under `network`

>[!note]
>This part still need to be written.
