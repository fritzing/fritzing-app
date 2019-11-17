/****************************************************************************
** Meta object code from reading C++ file 'testquazipdir.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../testquazipdir.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'testquazipdir.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_TestQuaZipDir_t {
    QByteArrayData data[9];
    char stringdata0[84];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_TestQuaZipDir_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_TestQuaZipDir_t qt_meta_stringdata_TestQuaZipDir = {
    {
QT_MOC_LITERAL(0, 0, 13), // "TestQuaZipDir"
QT_MOC_LITERAL(1, 14, 14), // "entryList_data"
QT_MOC_LITERAL(2, 29, 0), // ""
QT_MOC_LITERAL(3, 30, 9), // "entryList"
QT_MOC_LITERAL(4, 40, 7), // "cd_data"
QT_MOC_LITERAL(5, 48, 2), // "cd"
QT_MOC_LITERAL(6, 51, 13), // "entryInfoList"
QT_MOC_LITERAL(7, 65, 9), // "operators"
QT_MOC_LITERAL(8, 75, 8) // "filePath"

    },
    "TestQuaZipDir\0entryList_data\0\0entryList\0"
    "cd_data\0cd\0entryInfoList\0operators\0"
    "filePath"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TestQuaZipDir[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       7,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   49,    2, 0x08 /* Private */,
       3,    0,   50,    2, 0x08 /* Private */,
       4,    0,   51,    2, 0x08 /* Private */,
       5,    0,   52,    2, 0x08 /* Private */,
       6,    0,   53,    2, 0x08 /* Private */,
       7,    0,   54,    2, 0x08 /* Private */,
       8,    0,   55,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void TestQuaZipDir::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TestQuaZipDir *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->entryList_data(); break;
        case 1: _t->entryList(); break;
        case 2: _t->cd_data(); break;
        case 3: _t->cd(); break;
        case 4: _t->entryInfoList(); break;
        case 5: _t->operators(); break;
        case 6: _t->filePath(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

QT_INIT_METAOBJECT const QMetaObject TestQuaZipDir::staticMetaObject = { {
    &QObject::staticMetaObject,
    qt_meta_stringdata_TestQuaZipDir.data,
    qt_meta_data_TestQuaZipDir,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *TestQuaZipDir::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TestQuaZipDir::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_TestQuaZipDir.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int TestQuaZipDir::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 7;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
