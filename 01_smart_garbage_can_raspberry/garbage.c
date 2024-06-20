#include <Python.h>

void garbage_init(void)
{
    Py_Initialize();
    PyObject *sys = PyImport_ImportModule("sys");
    PyObject *path = PyObject_GetAttrString(sys, "path");
    PyList_Append(path, PyUnicode_FromString("."));
}

void garbage_final(void)
{
    Py_Finalize();
}

char *garbage_category(char *category)
{
    PyObject *pModule = PyImport_ImportModule("garbage");   // 相当于 import garbage (将garbage.py导入)
    if (!pModule) {
        PyErr_Print();
        printf("Error: failed to load garbage.py\n");
        goto FAILED_MODULE;
    }

    PyObject *pFunc = PyObject_GetAttrString(pModule, "alibaba_garbage");  // 导入里面的这个函数
    if (!pFunc) {
        PyErr_Print();
        printf("Error: failed to load alibabacloud_garbage\n");
        goto FAILED_FUNC;
    }

    PyObject *pValue = PyObject_CallObject(pFunc, NULL);    // 执行该函数
    if (!pValue) {
        PyErr_Print();
        printf("Error: function call failed\n");
        goto FAILED_VALUE;
    }

    char *result = NULL;                                    // 转为c语言格式
    if (!PyArg_Parse(pValue, "s", &result)) {
        PyErr_Print();
        printf("Error: parse failed");
        goto FAILED_RESULT;
    }
    category = (char *)malloc(sizeof(char) * (strlen(result) + 1));
    memset(category, 0, (strlen(result) + 1));
    strncpy(category, result, (strlen(result) + 1));        // 将获得数据放到category中

FAILED_RESULT:
    Py_DECREF(pValue);

FAILED_VALUE:
    Py_DECREF(pFunc);

FAILED_FUNC:
    Py_DECREF(pModule);

FAILED_MODULE:
    return category;
}