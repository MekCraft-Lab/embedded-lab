/**
 *******************************************************************************
 * @file    cmd-file.cpp
 * @brief   简要描述
 *******************************************************************************
 * @attention
 *
 * none
 *
 *******************************************************************************
 * @note
 *
 * none
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/8/26
 * @version 1.0
 *******************************************************************************
 */


/* ------- define --------------------------------------------------------------------------------------------------*/


/* ------- include ---------------------------------------------------------------------------------------------------*/

#include "cmd-file.h"
#include <sstream>
#include <string>

/* ------- class prototypes-----------------------------------------------------------------------------------------*/


static ConsoleApp& console = ConsoleApp::instance();

class PathManager {
  public:
    // 初始化当前路径
    PathManager() {}

    // 获取当前路径
    std::string getCurrentPath() const { return _currentPath; }

    void changePath(const PathManager& pm) {
        _currentPath = pm.getCurrentPath();
        _paths.assign(pm._paths.begin(), pm._paths.end());
    }

    bool changePath(std::string& newPath) {

        auto ret = setCurrentPath(newPath);
        std::string temp;
        for (auto it : _paths) {
            temp += "/";
            temp += it;
        }
        temp += "/";
        temp += _fileName;
        _currentPath = temp;
        return ret;
    }

    // 设置当前路径，根据相对路径更新
    bool setCurrentPath(std::string& path) {

        std::string part;
        char signal = 0;
        std::string temp;
        std::vector<std::string> tempSet;

        if (path.at(0) == '\\' || path.at(0) == '/') {
            _paths.clear();
            signal = path.at(0);
            path.erase(0,1);
        }


        for (auto ch : path) {
            uint8_t lastCh;
            if (signal == 0 and (ch == '\\' or ch == '/')) {
                signal = ch;
            }
            if (ch == signal) {
                if (lastCh == signal) {
                    return false;
                }
                tempSet.push_back(temp);
                temp.clear();
            } else if (ch == '\\' or ch == '/') {
                return false;
            } else {
                temp.push_back(ch);
            }
            lastCh = ch;
        }

        if (path.back() != signal) {
            _fileName = temp;
        } else {
            _fileName.clear();
        }

        if (tempSet.size() == 0) {
            return true;
        }

        for (const auto& it : tempSet) {
            if (it != ".") {
                if (it == "..") {
                    if (_paths.empty()) {
                        return false;
                    }
                    _paths.pop_back();
                } else {
                    _paths.push_back(it);
                }
            }
        }
        return true;
    }

    uint8_t getDepth() const { return _paths.size(); }

    std::string getFileName() const { return _fileName; }

    std::vector<std::string> getPaths() const { return _paths; }

  private:
    std::string _currentPath = "/"; // 当前路径
    std::string _fileName;
    std::vector<std::string> _paths;
};



/* ------- macro -----------------------------------------------------------------------------------------------------*/


/* ------- variables -------------------------------------------------------------------------------------------------*/

static uint8_t fileBuffer[1024];
static char currentPath[64] = "/";


static PathManager seeker, pathManager;


/* ------- function implement ----------------------------------------------------------------------------------------*/

std::string getFilePath(std::string path) {

    seeker.changePath(pathManager);

    if (!seeker.changePath(path)){
        console.error("ERROR: Failed to set current path");
        return "";
    }

    console.println("the absolute path: %s", seeker.getCurrentPath().c_str());
    return seeker.getCurrentPath();
}

int cat(int argc, char* argv[]) {
    if (argc < 2) {
        console.error("ERROR: Too few arguments");
        return -1;
    }

    std::string paramPath = argv[1];
    std::string path =  getFilePath(paramPath);
    if (path.empty()) {
        console.error("ERROR: Path is error");
        return -1;
    }
    memset(fileBuffer, 0, sizeof(fileBuffer));
    auto len = FileApp::instance().readFile(path.c_str(), fileBuffer, 1024);

    if (len > 0) {
        fileBuffer[len + 1] = 0;
        console.println("\r\n\r\n%s\r\n\r\n", fileBuffer);
    }
    return len;
}

int echo(int argc, char* argv[]) {
    if (argc < 4) {
        logPrintln("ERROR: Too few arguments");
        return -1;
    }

    if (argv[2][0] != '>') {
        logPrintln("ERROR: param wrong");
    }

    uint16_t len = 0;
    for (int i = 0; argv[1][i] != '\0'; i++) {
        len = i + 1;
    }
    logPrintln("len: %d", len);

    std::string paramPath = argv[3];

    std::string path =  getFilePath(paramPath);
    if (path.empty()) {
        console.error("ERROR: Path is error");
        return -1;
    }

    if (argv[2][1] == '>') {
        return FileApp::instance().appendFile(path.c_str(), argv[1], len);
    }
    return FileApp::instance().writeFile(path.c_str(), argv[1], len);
}

int touch(int argc, char* argv[]) {
    if (argc < 2) {
        logPrintln("ERROR: Too few arguments");
        return -1;
    }

    std::string paramPath = argv[1];
    std::string path =  getFilePath(paramPath);
    if (path.empty()) {
        console.error("ERROR: Path is error");
        return -1;
    }
    return FileApp::instance().createFile(path.c_str());
}

int ls(int argc, char* argv[]) {
    memset(fileBuffer, 0, sizeof(fileBuffer));
    char* path = nullptr;

    if (argc < 2) {
        path = currentPath;

    } else {
        path = argv[1];
    }

    auto ret = FileApp::instance().readDir(reinterpret_cast<const char*>(path), fileBuffer, 1024);

    if (ret < 0) {
        return ret;
    }

    console.println(
        "\r\n\r\n Direction: %s \r\n\r\ntype  name                     size\r\n====  =================        "
        "========\r\n",
        path);

    fileBuffer[ret++] = '\r';
    fileBuffer[ret++] = '\n';
    fileBuffer[ret]   = 0;

    console.output(reinterpret_cast<char*>(fileBuffer), ret);

    return 0;
}

int cd(int argc, char* argv[]) {
    //     memset(fileBuffer, 0, sizeof(fileBuffer));
    //
    //     if (argc < 2) {
    //         console.error("Too few arguments");
    //         return -1;
    //     }

    std::string nextPath = getFilePath(argv[1]);
    if (!seeker.getFileName().empty()) {
        console.error("format of path error");
        return -1;
    }
    if (FileApp::instance().openDir(nextPath.c_str()) < 0) {
        return -1;
    }

    std::string newPath = argv[1];
    if (pathManager.changePath(newPath)) {
        strcpy(currentPath, pathManager.getCurrentPath().c_str());
        shellSetPath(ShellApp::instance().getShell(), currentPath);
    } else {
        console.error("format of path error");
    }




    return 0;
}


int mkdir(int argc, char* argv[]) {

    memset(fileBuffer, 0, sizeof(fileBuffer));

    if (argc < 2) {
        console.error("Too few arguments");
        return -1;
    }

    std::string path =  getFilePath((argv[1]));
    if (path.empty()) {
        console.error("ERROR: Path is error");
        return -1;
    }
    console.println("absolute path : %s", path.c_str());

    auto ret = FileApp::instance().makeDir(path.c_str());


    if (ret < 0) {
        return ret;
    }
    // else {
    //     strcpy(reinterpret_cast<char*>(currentPath), argv[1]);
    //     shellSetPath(ShellApp::instance().getShell(), currentPath);
    // }
    return 0;
}

int rm(int argc, char* argv[]) {

    if (argc < 2) {
        console.error("Too few arguments");
        return -1;
    }

    std::string path =  getFilePath((argv[1]));

    auto ret = FileApp::instance().remove(path.c_str());

    return ret;
}

int move(int argc, char* argv[]) { return 0; }