#!/usr/bin/env python3
from ml import build
from ml.boilerplate import cpp
from ml import fileTools as ft
import os
import sys

libs = "/media/romain/Donnees/Programmation/cpp/libs"

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
fm.static = False
fm.includes = includes
fm.addToSrcs(srcs)
fm.addToLibs([
    "pthread", 
    "stdc++fs",
    "boost_filesystem",
    libs + "/FreeImagePlus-build/libfreeimageplus.a",
    "fmod",
    "fmodL"
    ])

fm.srcs_exclude += ["main.cpp"]
fm.shared = True
fm.definitions += ["NO_LOGS"]

fm.flags += ["-std=c++17"]
#if not fm.release : 
#    fm.flags += ["-fsanitize=thread"]

if ("clear" in sys.argv):
    fm.clean()
    exit()

fm.build()
