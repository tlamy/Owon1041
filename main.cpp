#include "MainWindow.h"
#include <QApplication>
#include <QPushButton>

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  MainWindow mainWindow;
  mainWindow.show();
  return QApplication::exec();
}
