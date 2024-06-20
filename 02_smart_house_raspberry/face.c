#include <Python.h>

void face_init(void)
{
    Py_Initialize();
    PyObject *sys = PyImport_ImportModule("sys");
    PyObject *path = PyObject_GetAttrString(sys, "path");
    PyList_Append(path, PyUnicode_FromString("."));
}

void face_final(void)
{
    Py_Finalize();
}

double face_category(void)
{
    double result = 0.0;   
    
    PyObject *pModule = PyImport_ImportModule("face");   // 相当于 import face (将face.py导入)
    if (!pModule) {
        PyErr_Print();
        printf("Error: failed to load face.py\n");
        goto FAILED_MODULE;
    }

    PyObject *pFunc = PyObject_GetAttrString(pModule, "alibaba_face");  // 导入face.py里面的alibaba_face函数
    if (!pFunc) {
        PyErr_Print();
        printf("Error: failed to load alibabacloud_face\n");
        goto FAILED_FUNC;
    }

    PyObject *pValue = PyObject_CallObject(pFunc, NULL);    // 执行该函数
    if (!pValue) {
        PyErr_Print();
        printf("Error: function call failed\n");
        goto FAILED_VALUE;
    }
                                
    if (!PyArg_Parse(pValue, "d", &result)) {               // 转为c语言格式，获取该数值
        PyErr_Print();
        printf("Error: parse failed");
        goto FAILED_RESULT;
    }

FAILED_RESULT:
    Py_DECREF(pValue);

FAILED_VALUE:
    Py_DECREF(pFunc);

FAILED_FUNC:
    Py_DECREF(pModule);

FAILED_MODULE:
    return result;
}