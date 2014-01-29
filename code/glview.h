#ifndef GLVIEWWIDGET_H
#define GLVIEWWIDGET_H

#include <QtOpenGL>
#include <QGLWidget>

class Glview : public QGLWidget
{
    Q_OBJECT

private:
    QTimer *t_Timer;

public:
    Glview(int framesPerSecond = 0, QWidget *parent = 0);
    void initializeGL() = 0;
    void resizeGL(int width, int height) = 0;
    void paintGL() = 0;

public slots:
    void timeOutSlot();

};


#endif // GLVIEWWIDGET_H