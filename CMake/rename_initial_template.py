import sys, os, datetime

templatename = input("Template name:")
templatename = templatename.replace(" ", "")


def genTemplates():
    global templatename

    files = [
        "CMakeLists.txt",
        "Jenkinsfile",
        ".github/workflows/autobuild.yml",
        "CMake/CMakeLists.txt",
        "CMake/Configure.cmake",
        "Source/CMakeLists.txt",
        "Test/CMakeLists.txt",
    ]

    patterns = [".yml", ".txt", ".cmake", "Jenkinsfile"]
    root = os.getcwd()

    for file in files:
        full_path = root + os.sep + file
        full_path = full_path.replace("\\", os.sep)

        if os.path.isfile(full_path):

            found = False
            for pat in patterns:
                if full_path.find(pat) != -1:
                    found = True
                    break
            if found:
                print("Found", full_path)
                fp = open(full_path, "r")
                buf = fp.read()
                fp.close()
                buf = buf.replace("Template", templatename)
                fp = open(full_path, "w")
                fp.write(buf)
                fp.close()


genTemplates()
