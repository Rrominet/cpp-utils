#!/usr/bin/env python3
from ml import docker
from ml import log
import sys

if not docker.isFromADocker() and "test" not in sys.argv : 
    log.print("You need to run this script inside a docker container, You're building for your machine host right now ! Abort.", "red")
    exit(1)

from ml import build
from ml.boilerplate import cpp
from ml import fileTools as ft
import os

libs = "../../libs"

for arg in sys.argv:
    if "libs=" in arg:
        libs = arg.split("=")[1]

if libs[-1] == "/":
    libs = libs[:-1]

cpp.generate("..")
includes = [
        "..",
        libs + "/json",
        libs + "/eigen",
        libs + "/sha3",
        libs + "/FreeImage/Source",
        libs + "/FreeImage/Wrapper/FreeImagePlus",
        libs + "/fmodstudioapi20000linux/api/core/inc",
        ]

srcs = [
        "..",
        "../files.2",
        "../network",
        "../observers",
        "../commands",
        "../sound",
        "../md4c",
        libs + "/sha3/sha3.c",
        ]


fm = build.create("mlapi", sys.argv)
fm.includes = includes
fm.addToSrcs(srcs)
fm.addToLibs([
    "pthread", 
    "stdc++fs",
    "boost_filesystem",
    libs + "/FreeImagePlus-build/libfreeimageplus.a",
    "libfmod.so",
    "libfmodL.so"
    ])

fm.srcs_exclude += ["main.cpp"]
fm.outputType = build.STATIC_LIB
fm.definitions += ["NO_LOGS"]

fm.flags += ["-std=c++17"]
#if not fm.release : 
#    fm.flags += ["-fsanitize=thread"]

if ("clear" in sys.argv):
    fm.clean()
    exit()

fm.build()
