/****************************************************************************
** Meta object code from reading C++ file 'testjlcompress.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../testjlcompress.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'testjlcompress.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_TestJlCompress_t {
    QByteArrayData data[15];
    char stringdata0[212];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_TestJlCompress_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_TestJlCompress_t qt_meta_stringdata_TestJlCompress = {
    {
QT_MOC_LITERAL(0, 0, 14), // "TestJlCompress"
QT_MOC_LITERAL(1, 15, 17), // "compressFile_data"
QT_MOC_LITERAL(2, 33, 0), // ""
QT_MOC_LITERAL(3, 34, 12), // "compressFile"
QT_MOC_LITERAL(4, 47, 18), // "compressFiles_data"
QT_MOC_LITERAL(5, 66, 13), // "compressFiles"
QT_MOC_LITERAL(6, 80, 16), // "compressDir_data"
QT_MOC_LITERAL(7, 97, 11), // "compressDir"
QT_MOC_LITERAL(8, 109, 16), // "extractFile_data"
QT_MOC_LITERAL(9, 126, 11), // "extractFile"
QT_MOC_LITERAL(10, 138, 17), // "extractFiles_data"
QT_MOC_LITERAL(11, 156, 12), // "extractFiles"
QT_MOC_LITERAL(12, 169, 15), // "extractDir_data"
QT_MOC_LITERAL(13, 185, 10), // "extractDir"
QT_MOC_LITERAL(14, 196, 15) // "zeroPermissions"

    },
    "TestJlCompress\0compressFile_data\0\0"
    "compressFile\0compressFiles_data\0"
    "compressFiles\0compressDir_data\0"
    "compressDir\0extractFile_data\0extractFile\0"
    "extractFiles_data\0extractFiles\0"
    "extractDir_data\0extractDir\0zeroPermissions"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TestJlCompress[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      13,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   79,    2, 0x08 /* Private */,
       3,    0,   80,    2, 0x08 /* Private */,
       4,    0,   81,    2, 0x08 /* Private */,
       5,    0,   82,    2, 0x08 /* Private */,
       6,    0,   83,    2, 0x08 /* Private */,
       7,    0,   84,    2, 0x08 /* Private */,
       8,    0,   85,    2, 0x08 /* Private */,
       9,    0,   86,    2, 0x08 /* Private */,
      10,    0,   87,    2, 0x08 /* Private */,
      11,    0,   88,    2, 0x08 /* Private */,
      12,    0,   89,    2, 0x08 /* Private */,
      13,    0,   90,    2, 0x08 /* Private */,
      14,    0,   91,    2, 0x08 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void TestJlCompress::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TestJlCompress *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->compressFile_data(); break;
        case 1: _t->compressFile(); break;
        case 2: _t->compressFiles_data(); break;
        case 3: _t->compressFiles(); break;
        case 4: _t->compressDir_data(); break;
        case 5: _t->compressDir(); break;
        case 6: _t->extractFile_data(); break;
        case 7: _t->extractFile(); break;
        case 8: _t->extractFiles_data(); break;
        case 9: _t->extractFiles(); break;
        case 10: _t->extractDir_data(); break;
        case 11: _t->extractDir(); break;
        case 12: _t->zeroPermissions(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

QT_INIT_METAOBJECT const QMetaObject TestJlCompress::staticMetaObject = { {
    &QObject::staticMetaObject,
    qt_meta_stringdata_TestJlCompress.data,
    qt_meta_data_TestJlCompress,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *TestJlCompress::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TestJlCompress::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_TestJlCompress.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int TestJlCompress::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 13)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 13;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
