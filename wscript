#!/bin/python

from waflib import *
import os, sys

top = '.'
out = 'build'

projname = 'blueshift'

coreprog_name = projname
coremod_name = projname + "_module"

g_cflags = ["-Wall", "-Wextra", "-std=c++17"]
def btype_cflags(ctx):
	return {
		"DEBUG"   : g_cflags + ["-Og", "-ggdb3", "-march=core2", "-mtune=native"],
		"NATIVE"  : g_cflags + ["-Ofast", "-march=native", "-mtune=native"],
		"RELEASE" : g_cflags + ["-O3", "-march=core2", "-mtune=generic"],
	}.get(ctx.env.BUILD_TYPE, g_cflags)

def options(opt):
	opt.load("g++")
	opt.add_option('--build_type', dest='build_type', type="string", default='RELEASE', action='store', help="DEBUG, NATIVE, RELEASE")

def configure(ctx):
	ctx.load("g++")
	ctx.check(features='c cprogram', lib='pthread', uselib_store='PTHREAD')
	ctx.check(features='c cprogram', lib='dl', uselib_store='DL')
	btup = ctx.options.build_type.upper()
	if btup in ["DEBUG", "NATIVE", "RELEASE"]:
		Logs.pprint("PINK", "Setting up environment for known build type: " + btup)
		ctx.env.BUILD_TYPE = btup
		ctx.env.CXXFLAGS = btype_cflags(ctx)
		Logs.pprint("PINK", "CXXFLAGS: " + ' '.join(ctx.env.CXXFLAGS))
		if btup == "DEBUG":
			ctx.define("BLUESHIFT_DEBUG", 1)
	else:
		Logs.error("UNKNOWN BUILD TYPE: " + btup)
		
def build(bld):
	
	mod_install_files = bld.path.ant_glob('src/module/*.hh')
	bld.install_files('${PREFIX}/include/blueshift', mod_install_files)
	
	bluemod_files = bld.path.ant_glob('src/module/*.cc')
	bluemod = bld (
		features = "cxx cxxshlib",
		target = coremod_name,
		source = bluemod_files,
		uselib = [],
		includes = [os.path.join(top, 'src', 'module')],
	)
	
	coreprog_files = bld.path.ant_glob('src/*.cc')
	coreprog = bld (
		features = "cxx cxxprogram",
		target = coreprog_name,
		source = coreprog_files,
		use = [coremod_name],
		uselib = ['PTHREAD', 'DL'],
		includes = [os.path.join(top, 'src'), os.path.join(top, 'src', 'module')],
	)
