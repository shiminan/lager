//
// lager - library for functional interactive c++ programs
// Copyright (C) 2017 Juan Pedro Bolivar Puente
//
// This file is part of lager.
//
// lager is free software: you can redistribute it and/or modify
// it under the terms of the MIT License, as detailed in the LICENSE
// file located at the root of this source code distribution,
// or here: <https://github.com/arximboldi/lager/blob/master/LICENSE>
//

#include "../todo.hpp"

#include <lager/cursor.hpp>
#include <lager/extra/qt.hpp>
#include <lager/state.hpp>

#include <QApplication>
#include <QFileInfo>
#include <QMessageBox>
#include <QObject>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

#include <iostream>

class Entry : public QObject
{
    Q_OBJECT

public:
    Entry(lager::cursor<todo::entry> data)
        : LAGER_QT(done){data[&todo::entry::done]}
        , LAGER_QT(text){data[&todo::entry::text].xf(
              zug::map([](auto&& x) { return QString::fromStdString(x); }),
              zug::map([](auto&& x) { return x.toStdString(); }))}
    {}

    LAGER_QT_CURSOR(bool, done);
    LAGER_QT_CURSOR(QString, text);
};

class Model : public QObject
{
    Q_OBJECT

    lager::state<todo::model> state_;
    lager::state<QString> file_name_;

public:
    Model()
        : LAGER_QT(name){state_[&todo::model::name].xf(
              zug::map([](auto&& x) { return QString::fromStdString(x); }),
              zug::map([](auto&& x) { return x.toStdString(); }))}
        , LAGER_QT(fileName){file_name_}
        , LAGER_QT(count){state_.xf(zug::map(
              [](auto&& x) { return static_cast<int>(x.todos.size()); }))}
    {}

    LAGER_QT_READER(QString, name);
    LAGER_QT_READER(QString, fileName);
    LAGER_QT_READER(int, count);

    Q_INVOKABLE Entry* todo(int index)
    {
        return new Entry{state_[&todo::model::todos][index]};
    }

    Q_INVOKABLE void add(QString text)
    {
        state_.update([&](auto x) {
            x.todos = x.todos.push_front({false, text.toStdString()});
            return x;
        });
    }

    Q_INVOKABLE void remove(int index)
    {
        state_.update([&](auto x) {
            x.todos = x.todos.erase(index);
            return x;
        });
    }

    Q_INVOKABLE bool save(QString fname)
    {
        try {
            auto fpath = QUrl{fname}.toLocalFile();
            if (QFileInfo{fname}.suffix() != "todo")
                fpath += ".todo";
            todo::save(fpath.toStdString(), state_.get());
            state_.update([&](auto s) {
                s.name = QFileInfo{fname}.baseName().toStdString();
                return s;
            });
            file_name_.set(fname);
            commit();
            return true;
        } catch (std::exception const& err) {
            std::cerr << "Exception thrown: " << err.what() << std::endl;
            return false;
        }
    }

    Q_INVOKABLE bool load(QString fname)
    {
        try {
            auto model = todo::load(QUrl{fname}.toLocalFile().toStdString());
            model.name = QFileInfo{fname}.baseName().toStdString();
            state_.set(model);
            file_name_.set(fname);
            commit();
            return true;
        } catch (std::exception const& err) {
            std::cerr << "Exception thrown: " << err.what() << std::endl;
            return false;
        }
    }

    Q_INVOKABLE void commit() { lager::commit(state_, file_name_); }
};

#include "main.moc"

int main(int argc, char** argv)
{
    QApplication app{argc, argv};
    QQmlApplicationEngine engine;

    qmlRegisterType<Model>("Lager.Example.Todo", 1, 0, "Model");
    qmlRegisterUncreatableType<Entry>("Lager.Example.Todo", 1, 0, "Entry", "");

    QQuickStyle::setStyle("Material");

    engine.load(LAGER_TODO_QML_DIR "/main.qml");

    return app.exec();
}
