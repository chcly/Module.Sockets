# -----------------------------------------------------------------------------
#
#   Copyright (c) Charles Carley.
#
#   This software is provided 'as-is', without any express or implied
# warranty. In no event will the authors be held liable for any damages
# arising from the use of this software.
#
#   Permission is granted to anyone to use this software for any purpose,
# including commercial applications, and to alter it and redistribute it
# freely, subject to the following restrictions:
#
# 1. The origin of this software must not be misrepresented; you must not
#    claim that you wrote the original software. If you use this software
#    in a product, an acknowledgment in the product documentation would be
#    appreciated but is not required.
# 2. Altered source versions must be plainly marked as such, and must not be
#    misrepresented as being the original software.
# 3. This notice may not be removed or altered from any source distribution.
# ------------------------------------------------------------------------------
import os, subprocess, re

PULL = "git pull"


def trim(line):
    line = line.replace('\t', '')
    line = line.replace('\n', '')
    line = line.replace(' ', '')
    return line


def makeAbsolute(currentDir, relativeDir):
    return os.path.join(currentDir, relativeDir).replace("\\", "/")


def checkUrl(url):
    matchObj = re.match("[a-zA-Z0-9:/.]+", url)
    if (matchObj != None):
        return url
    return None


def checkBranch(branch):
    matchObj = re.match("[a-zA-Z0-9.]+", branch)
    if (matchObj != None):
        return branch
    return None


def checkShallow(shallow):
    if (shallow == "true" or shallow == "false"):
        return shallow
    return None


def execProgram(args):
    subprocess.run(args.split(' '),
                   shell=False,
                   env=os.environ,
                   capture_output=True)


def changeDirectory(directory):

    try:
        os.chdir(directory)
        val = os.getcwd()
        directory = directory.replace('\\', '/')
        val = val.replace('\\', '/')
        return directory == val
    except OSError:
        pass
    return False


def initModules():
    # this is meant to be called from the same directory
    # as the module script.

    execProgram(PULL)
    execProgram("git submodule init")
    execProgram("git submodule update --init --merge")


def updateModules(moduleDict):

    for key in moduleDict.keys():
        module = moduleDict[key]

        path = module.get("path", None)
        branch = module.get("branch", None)
        shallow = module.get("shallow", None)
        url = module.get("url", None)

        print("Updating", url)
        print("=>".rjust(11, ' '), module.get("name", None))

        if (path != None):
            if (not os.path.isdir(path)):
                print(
                    "could not determine the directory for the "
                    "supplied path:", path)
                continue

            if (url == None):
                continue

            if (not changeDirectory(path)):
                print(
                    "could not switch directory "
                    "to the module at the supplied path:", path)
                continue

            branchStr = "master"
            if (branch != None):
                branchStr = branch

            shallowValue = False
            if (shallow != None):
                shallowValue = (shallow == 'true')

            if (shallowValue == False):
                execProgram("git checkout %s" % branchStr)
                execProgram(PULL)
            else:
                execProgram("git checkout -f -B %s" % branchStr)
                execProgram(PULL + " %s %s" % (url, branchStr))


def collectModules(currentDir, gitModulesFile):

    moduleDict = {}
    fp = open(makeAbsolute(currentDir, gitModulesFile), mode='r')
    lines = fp.readlines()
    fp.close()

    # this assumes that variables come after the path
    moduleName = None

    for line in lines:
        line = trim(line)
        subModuleCode = "[submodule\""
        if (line.startswith(subModuleCode)):
            moduleName = line[len(subModuleCode):-2]
            moduleDict[moduleName] = {}
            variableDict = moduleDict[moduleName]
            variableDict['hasUrl'] = False

        elif (moduleName != None):
            # exclude everything before
            variableDict = moduleDict[moduleName]

            if (line.find("url=") != -1):
                variableDict["url"] = checkUrl(line.replace("url=", ''))
                variableDict['hasUrl'] = True
            elif (line.find("branch=") != -1):
                variableDict["branch"] = checkBranch(
                    line.replace("branch=", ''))
            elif (line.find("shallow=") != -1):
                variableDict["shallow"] = checkShallow(
                    line.replace("shallow=", ''))
            elif (line.find("path=") != -1):
                moduleDirectory = line.replace("path=", '')
                absPath = None
                if (os.path.isabs(moduleDirectory)):
                    absPath = moduleDirectory
                else:
                    absPath = makeAbsolute(currentDir, moduleDirectory)

                if (os.path.isdir(absPath)):
                    variableDict["path"] = absPath
                    variableDict["name"] = moduleDirectory
            else:
                if (len(line) > 0):
                    print("unhandled line: ", line)

    for key in moduleDict.keys():
        module = moduleDict[key]
        if (module.get("hasUrl", False) == False):
            print("unable to determine the url for the module:", key)

    return moduleDict


def main():
    currentDir = os.getcwd()
    gitModulesFile = os.path.abspath(".gitmodules")
    if (not os.path.isfile(gitModulesFile)):
        print("No .gitmodule found in", currentDir, "\nNothing to update...")
        return
    moduleDict = collectModules(currentDir, gitModulesFile)

    initModules()
    updateModules(moduleDict)
    changeDirectory(currentDir)


if __name__ == '__main__':
    main()