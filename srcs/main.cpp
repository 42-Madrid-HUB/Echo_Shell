#include "shell.hpp"
#include "log.hpp"

int main() {
    // Configure logger
    esh::Logger& L = esh::Logger::instance();
    L.setLevel(esh::Logger::Level::Info);
    L.setPattern("[{time}] {level} {file}:{line} {func} | {msg}");
    L.setFile(".exam-shell.log");          // creates/append in CWD
    L.setRotation(1024 * 1024, 5);         // 1MB, keep 5 files
    L.setAsync(true);

    ESH_LOG_INFO() << "Exam shell starting";

    Shell shell;
    shell.run();

    ESH_LOG_INFO() << "Exam shell exited";
    L.flush();
    return 0;
}
