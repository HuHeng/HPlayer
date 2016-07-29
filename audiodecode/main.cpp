#include <QApplication>
#include "audioplayer.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    AudioPlayer ap;
    ap.openAudioFile();
    ap.play();
    ap.show();
    return a.exec();
}
