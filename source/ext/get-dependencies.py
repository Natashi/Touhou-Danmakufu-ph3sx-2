# This file is part of ph3sx (https://github.com/Natashi/Touhou-Danmakufu-ph3sx-2)
#
# Run this script to fetch the dependencies required for compiling ph3sx.

import os
import sys
import shutil
import glob

def ExecSh(cmd, p=True):
	if p:
		print(cmd)
	res = os.popen(cmd).read()
	return res

# ----------------------------------------------------------------


map_libraries = {
	'libogg': 		('ogg', 'https://github.com/xiph/ogg', 'v1.3.4', 'bada457'),
	'libvorbis': 	('vorbis', 'https://github.com/xiph/vorbis', 'v1.3.6', 'd22c3ab'),
	'zlib':			('zlib', 'https://github.com/madler/zlib', 'v1.2.11', 'cacf7f1'),
	'kissfft': 		('kissfft', 'https://github.com/mborgerding/kissfft', 'v131', '0bfa4d5'),
	'imgui': 		('imgui', 'https://github.com/ocornut/imgui', 'v1.89.1', 'a8df192'),
}


DIR_TEMP = '.tmp'

orgDir = os.getcwd()
if not os.path.isdir(DIR_TEMP):
	os.mkdir(DIR_TEMP)

def CloneAndCheckVersion(name):
	if name not in map_libraries:
		print(f'No library "{name}"')
		return
	
	targetDir, repoUrl, versionTag, commitID = map_libraries[name]
	
	print(f'Checking for {name} {versionTag}...\n')

	def Clone():
		ExecSh(f"git clone --depth 1 --branch {versionTag} {repoUrl}")

	os.chdir(DIR_TEMP)
	if not os.path.isdir(targetDir):
		Clone()
	else:
		rid = ExecSh(f"git -C {targetDir} rev-parse --short HEAD", p=False)
		if rid.strip() != commitID:
			print(f'Incorrect {name} version (target={versionTag}), re-acquiring...\n')
			
			def _OnError(func, path, exc_info):
				import stat
				if not os.access(path, os.W_OK):
					os.chmod(path, stat.S_IWUSR)
					func(path)
				else:
					raise
			shutil.rmtree(targetDir, ignore_errors=False, onerror=_OnError)
			
			Clone()
		else:
			print(f'{name} up to date\n')
	os.chdir('..')

# ----------------------------------------------------------------
# libogg

CloneAndCheckVersion('libogg')

if not os.path.isdir('ogg'):
	os.mkdir("ogg")

for i in ['ogg.h', 'os_types.h']:
	f = DIR_TEMP + '/ogg/include/ogg/' + i
	print('Copying ' + f)
	shutil.copy2(f, 'ogg/')

print()

# ----------------------------------------------------------------
# libvorbis

CloneAndCheckVersion('libvorbis')

if not os.path.isdir('vorbis'):
	os.mkdir("vorbis")

for i in ['codec.h', 'vorbisfile.h']:
	f = DIR_TEMP + '/vorbis/include/vorbis/' + i
	print('Copying ' + f)
	shutil.copy2(f, 'vorbis/')

print()

# ----------------------------------------------------------------
# zlib

CloneAndCheckVersion('zlib')

if not os.path.isdir('zlib'):
	os.mkdir("zlib")

for i in ['zconf.h', 'zlib.h']:
	f = DIR_TEMP + '/zlib/' + i
	print('Copying ' + f)
	shutil.copy2(f, 'zlib/')

print()

# ----------------------------------------------------------------
# kissfft

CloneAndCheckVersion('kissfft')

if not os.path.isdir('kissfft'):
	os.mkdir("kissfft")

for i in ['kissfft.hh']:
	f = DIR_TEMP + '/kissfft/' + i
	print('Copying ' + f)
	shutil.copy2(f, 'kissfft/')

print()

# ----------------------------------------------------------------
# imgui

CloneAndCheckVersion('imgui')

IMGUI_DIR = '.tmp/imgui/'

if not os.path.isdir('imgui'):
	os.mkdir("imgui")
if not os.path.isdir('imgui/backends'):
	os.mkdir("imgui/backends")
if not os.path.isdir('imgui/misc'):
	os.mkdir("imgui/misc")

files = glob.glob(IMGUI_DIR + '*.h') + glob.glob(IMGUI_DIR + '*.cpp')
for i in files:
	if 'demo' in i:
		continue
	print('Copying ' + i)
	shutil.copy2(i, 'imgui/')

files = glob.glob(IMGUI_DIR + 'backends/imgui_impl_win32.*') \
	+ glob.glob(IMGUI_DIR + 'backends/imgui_impl_dx9.*')
for i in files:
	print('Copying ' + i)
	shutil.copy2(i, 'imgui/backends/')

for i in ['cpp', 'debuggers', 'freetype', 'single_file']:
	if not os.path.isdir('imgui/misc/' + i):
		os.mkdir('imgui/misc/' + i)
	
	files = glob.glob(IMGUI_DIR + 'misc/' + i + '/*')
	for iFile in files:
		print('Copying ' + iFile)
		shutil.copy2(iFile, 'imgui/misc/' + i)

shutil.copy2(IMGUI_DIR + 'misc/README.txt', 'imgui/misc/')

print()