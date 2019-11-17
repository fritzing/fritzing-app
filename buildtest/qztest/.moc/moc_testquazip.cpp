/****************************************************************************
** Meta object code from reading C++ file 'testquazip.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.12.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../testquazip.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'testquazip.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.12.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_TestQuaZip_t {
    QByteArrayData data[18];
    char stringdata0[246];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_TestQuaZip_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_TestQuaZip_t qt_meta_stringdata_TestQuaZip = {
    {
QT_MOC_LITERAL(0, 0, 10), // "TestQuaZip"
QT_MOC_LITERAL(1, 11, 16), // "getFileList_data"
QT_MOC_LITERAL(2, 28, 0), // ""
QT_MOC_LITERAL(3, 29, 11), // "getFileList"
QT_MOC_LITERAL(4, 41, 8), // "add_data"
QT_MOC_LITERAL(5, 50, 3), // "add"
QT_MOC_LITERAL(6, 54, 21), // "setFileNameCodec_data"
QT_MOC_LITERAL(7, 76, 16), // "setFileNameCodec"
QT_MOC_LITERAL(8, 93, 14), // "setOsCode_data"
QT_MOC_LITERAL(9, 108, 9), // "setOsCode"
QT_MOC_LITERAL(10, 118, 31), // "setDataDescriptorWritingEnabled"
QT_MOC_LITERAL(11, 150, 16), // "testQIODeviceAPI"
QT_MOC_LITERAL(12, 167, 10), // "setZipName"
QT_MOC_LITERAL(13, 178, 11), // "setIoDevice"
QT_MOC_LITERAL(14, 190, 15), // "setCommentCodec"
QT_MOC_LITERAL(15, 206, 12), // "setAutoClose"
QT_MOC_LITERAL(16, 219, 11), // "saveFileBug"
QT_MOC_LITERAL(17, 231, 14) // "testSequential"

    },
    "TestQuaZip\0getFileList_data\0\0getFileList\0"
    "add_data\0add\0setFileNameCodec_data\0"
    "setFileNameCodec\0setOsCode_data\0"
    "setOsCode\0setDataDescriptorWritingEnabled\0"
    "testQIODeviceAPI\0setZipName\0setIoDevice\0"
    "setCommentCodec\0setAutoClose\0saveFileBug\0"
    "testSequential"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_TestQuaZip[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      16,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags
       1,    0,   94,    2, 0x08 /* Private */,
       3,    0,   95,    2, 0x08 /* Private */,
       4,    0,   96,    2, 0x08 /* Private */,
       5,    0,   97,    2, 0x08 /* Private */,
       6,    0,   98,    2, 0x08 /* Private */,
       7,    0,   99,    2, 0x08 /* Private */,
       8,    0,  100,    2, 0x08 /* Private */,
       9,    0,  101,    2, 0x08 /* Private */,
      10,    0,  102,    2, 0x08 /* Private */,
      11,    0,  103,    2, 0x08 /* Private */,
      12,    0,  104,    2, 0x08 /* Private */,
      13,    0,  105,    2, 0x08 /* Private */,
      14,    0,  106,    2, 0x08 /* Private */,
      15,    0,  107,    2, 0x08 /* Private */,
      16,    0,  108,    2, 0x08 /* Private */,
      17,    0,  109,    2, 0x08 /* Private */,

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
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void TestQuaZip::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TestQuaZip *>(_o);
        Q_UNUSED(_t)
        switch (_id) {
        case 0: _t->getFileList_data(); break;
        case 1: _t->getFileList(); break;
        case 2: _t->add_data(); break;
        case 3: _t->add(); break;
        case 4: _t->setFileNameCodec_data(); break;
        case 5: _t->setFileNameCodec(); break;
        case 6: _t->setOsCode_data(); break;
        case 7: _t->setOsCode(); break;
        case 8: _t->setDataDescriptorWritingEnabled(); break;
        case 9: _t->testQIODeviceAPI(); break;
        case 10: _t->setZipName(); break;
        case 11: _t->setIoDevice(); break;
        case 12: _t->setCommentCodec(); break;
        case 13: _t->setAutoClose(); break;
        case 14: _t->saveFileBug(); break;
        case 15: _t->testSequential(); break;
        default: ;
        }
    }
    Q_UNUSED(_a);
}

QT_INIT_METAOBJECT const QMetaObject TestQuaZip::staticMetaObject = { {
    &QObject::staticMetaObject,
    qt_meta_stringdata_TestQuaZip.data,
    qt_meta_data_TestQuaZip,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *TestQuaZip::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *TestQuaZip::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_TestQuaZip.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int TestQuaZip::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 16)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 16;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 16)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 16;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
