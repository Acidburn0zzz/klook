TEMPLATE += app
QT += gui declarative phonon dbus opengl

DEFINES += QT_NO_DEBUG_OUTPUT

CONFIG += qt

LIBS += -lplasma \
        -lkio \
        -lkfile \
        -lkdecore

DEFINES += QMLJSDEBUGGER

OBJECTS_DIR = build/obj
MOC_DIR     = build/moc
RCC_DIR     = build/rcc
UI_DIR      = build/ui

TARGET = klook

SOURCES += main.cpp \
    declarativeviewer.cpp \
    video.cpp \ 
    text.cpp \
    file.cpp \
    workerthread.cpp \
    previewgenerator.cpp \
    previewprovider.cpp \
    filemodel.cpp \
    klookapp.cpp

HEADERS += \
    declarativeviewer.h \
    video.h \
    text.h \
    file.h \
    workerthread.h \
    previewgenerator.h \
    previewprovider.h \
    filemodel.h \
    klookapp.h

OTHER_FILES += \
    qml/klook/main.qml \
    qml/klook/Button.qml \
    qml/klook/images/window-close.png \
    qml/klook/images/view-restore.png \
    qml/klook/images/view-list-icons.png \
    qml/klook/images/view-fullscreen.png \
    qml/klook/images/go-previous.png \
    qml/klook/images/play.png \
    qml/klook/images/pause.png \
    qml/klook/images/go-next.png \
    qml/klook/buttonprev.qml \
    qml/klook/PlayButton.qml \
    qml/klook/images/play button.svg \
    qml/klook/Slider.qml \
    qml/klook/Delegate.qml \
    qml/klook/ControlPanel.qml \
    qml/klook/DefaultImage.qml \
    qml/klook/ControlPanelButton.qml \
    qml/klook/FullscreenPanel.qml \
    qml/klook/FullscreenPanelEx.qml \
    qml/klook/FullscreenPanelPlayEx.qml \
    qml/klook/FullscreenPanelPlay.qml \
    qml/klook/Preview.qml \
    qml/klook/ScrollBar.qml \
    qml/klook/DefaultImage.qml \
    qml/klook/images/play-empty.png \
    qml/klook/SingleDelegate.qml \
    resources/pla-empty-box.png \
    resources/play-empty.png

unix {
    HARDWARE_PLATFORM = $$system(uname -i)
    contains( HARDWARE_PLATFORM, x86_64 ) {
        LIBS_PATH = /usr/lib64
    } else {
        LIBS_PATH = /usr/lib
    }
}


target.path=/usr/bin

qml.path=$$LIBS_PATH/klook/
qml.files=qml/klook/*.qml

qm.path=$$LIBS_PATH/klook/translations
qm.files=translations/klook_ru.qm

icons.path=$$LIBS_PATH/klook/images/
icons.files=qml/klook/images/*.png

button_normal.path=$$LIBS_PATH/klook/images/buttons/normal/
button_normal.files=qml/klook/images/buttons/normal/*.png

button_hover.path=$$LIBS_PATH/klook/images/buttons/hover/
button_hover.files=qml/klook/images/buttons/hover/*.png

button_disable.path=$$LIBS_PATH/klook/images/buttons/disable/
button_disable.files=qml/klook/images/buttons/disable/*.png

button_press.path=$$LIBS_PATH/klook/images/buttons/press/
button_press.files=qml/klook/images/buttons/press/*.png

INSTALLS = target icons qml button_press button_disable button_hover button_normal

*-g++-32 {
message("Any 32-bit GCC build")
}

RESOURCES += \
    resources.qrc
