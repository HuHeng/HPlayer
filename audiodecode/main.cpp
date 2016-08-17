#include <QApplication>
#include "audioplayer.h"
#include <iostream>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    AudioPlayer ap;
    ap.openAudioFile();
    ap.show();
    std::cout<<std::this_thread::get_id()<<std::endl;
    return a.exec();
}
