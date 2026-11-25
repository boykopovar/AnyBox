#include "Tests.hpp"
#include <iostream>

int main() {
    try {
        AnyBox::Relinker::Tests::CheckWriteback();
        AnyBox::Relinker::Tests::CheckExecution();
        AnyBox::Relinker::Tests::CheckImages();
        AnyBox::Relinker::Tests::CheckCpu();
        AnyBox::Relinker::Tests::CheckControlFlow();
        AnyBox::Relinker::Tests::CheckInsert();
        AnyBox::Relinker::Tests::CheckPatching();
        AnyBox::Relinker::Tests::CheckExtract();
        AnyBox::Relinker::Tests::CheckMonitor();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
