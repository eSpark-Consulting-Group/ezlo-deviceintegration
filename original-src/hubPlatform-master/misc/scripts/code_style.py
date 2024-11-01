#!/usr/bin/env python3
import argparse
import glob
import re
import subprocess
import itertools
from multiprocessing import Pool
from os import path

fmt_tool = "clang-format"
required_fmt_version = 6
msg_match_style = "The files match code style :)"
extensions_regex = re.compile(r"(.*\.h$|.*\.cpp$|.*\.c$|.*\.hpp$)")
scriptFolder = path.dirname(path.realpath(__file__))
sourceRootFolder = path.join(scriptFolder, "..", "..")


def checkClangFormatVersion():
	try:
		cf_version = subprocess.check_output([fmt_tool, "--version"], encoding="utf-8")
		version_regex = re.compile(r"clang-format version ([0-9])\.")
		major_version_num = int(version_regex.match(cf_version).group(1))
		return major_version_num >= required_fmt_version
	except:
		return False


def normalizePaths(files):
	return map(lambda file: path.join(sourceRootFolder, file), files)


def filesListFromPaths(inputDirs):
	for directory in inputDirs:
		files_and_dirs = glob.glob(directory+'/'+"**", recursive=True)
		files = filter(path.isfile, files_and_dirs)
		files = filter(extensions_regex.match, files)
		yield from normalizePaths(files)


def allPaths():
	# List of allowed directectories by Andrey Stepanov for code style verification
	return [
		'deps/tiny',
		'deps/2GIG',
		'firmware'
	]


def getFilesFromGit(*command):
	files = subprocess.check_output(command, cwd=sourceRootFolder, encoding="utf-8").splitlines()
	files = filter(extensions_regex.match, files)
	return normalizePaths(files)


def branchDiffFiles():
	diff = getFilesFromGit("git", "diff", "--name-only", "--diff-filter=ACMR", "master")
	untracked = getFilesFromGit("git", "ls-files", "-o", "--exclude-standard")

	return itertools.chain(diff, untracked)


def stagedFiles():
	return getFilesFromGit("git", "diff", "--cached", "--name-only", "--diff-filter=ACMR")


def checkFilesFormatting(files, jobs):
	nonFormattedFilesList = getFilesMustBeFormatted(files, jobs)
	if nonFormattedFilesList:
		print("Must be formatted:")
		for file in nonFormattedFilesList:
			print(path.relpath(file))
		exit(1)
	else:
		print(msg_match_style)


def checkFileFormat(file):
	out = subprocess.check_output([fmt_tool, "-style", "file", "-output-replacements-xml", file])
	if "<replacement offset=" in str(out):
		return file


def getFilesMustBeFormatted(files, jobs):
	with Pool(processes=jobs) as pool:
		return list(filter(lambda file: file is not None, pool.imap(checkFileFormat, files, chunksize=10)))


def formatFiles(files, jobs):
	nonFormattedFilesList = getFilesMustBeFormatted(files, jobs)
	if nonFormattedFilesList:
		print("These files were formatted:")
		for file in nonFormattedFilesList:
			subprocess.run(["clang-format", "-style", "file", "-i", file])
			print(path.relpath(file))
	else:
		print(msg_match_style)
	exit(0)


def main():
	if not checkClangFormatVersion() and subprocess.call(['sudo', './install_clang_format.sh']):
		exit("clang-format version not supported, expected >= %d" % required_fmt_version)

	arguments_parser = argparse.ArgumentParser(description="Formatting the source code.")
	arguments_parser.add_argument("--action", help="Desired action.", choices=['check', 'format'], required=True)
	arguments_parser.add_argument("--all", help="Process all source files in the repository.", action='store_true')
	arguments_parser.add_argument("--staged", help="Process only staged files.", action='store_true')
	arguments_parser.add_argument("--paths", nargs='+', metavar="", help="""Paths to directories in which you need to 
		format or check the files code style formatting. """)
	arguments_parser.add_argument("-j", dest='jobs', type=int, default=1, help="Specifies the number of jobs to run simultaneously.")
	args = arguments_parser.parse_args()

	if args.all:
		files = filesListFromPaths(allPaths())
	elif args.paths is not None:
		files = filesListFromPaths(args.paths)
	elif args.staged:
		files = stagedFiles()
	else:
		files = branchDiffFiles()

	if args.action == 'check':
		checkFilesFormatting(files, args.jobs)
	else:
		formatFiles(files, args.jobs)


if __name__ == '__main__':
	exit(main())
