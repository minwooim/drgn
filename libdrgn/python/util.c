// Copyright 2018-2019 - Omar Sandoval
// SPDX-License-Identifier: GPL-3.0+

#include "drgnpy.h"

int append_string(PyObject *parts, const char *s)
{
	PyObject *str;
	int ret;

	str = PyUnicode_FromString(s);
	if (!str)
		return -1;

	ret = PyList_Append(parts, str);
	Py_DECREF(str);
	return ret;
}

static int append_formatv(PyObject *parts, const char *format, va_list ap)
{
	PyObject *str;
	int ret;

	str = PyUnicode_FromFormatV(format, ap);
	if (!str)
		return -1;

	ret = PyList_Append(parts, str);
	Py_DECREF(str);
	return ret;
}

int append_format(PyObject *parts, const char *format, ...)
{
	va_list ap;
	int ret;

	va_start(ap, format);
	ret = append_formatv(parts, format, ap);
	va_end(ap);
	return ret;
}

unsigned long long index_arg(PyObject *obj, const char *msg)
{
	if (PyLong_Check(obj)) {
		return PyLong_AsUnsignedLongLong(obj);
	} else if (PyIndex_Check(obj)) {
		PyObject *index_obj;
		unsigned long long ret;

		index_obj = PyNumber_Index(obj);
		if (!index_obj)
			return -1;
		ret = PyLong_AsUnsignedLongLong(index_obj);
		Py_DECREF(index_obj);
		return ret;
	} else {
		PyErr_SetString(PyExc_TypeError, msg);
		return -1;
	}
}

PyObject *byteorder_string(bool little_endian)
{
	_Py_IDENTIFIER(little);
	_Py_IDENTIFIER(big);
	PyObject *ret;

	ret = _PyUnicode_FromId(little_endian ? &PyId_little : &PyId_big);
	Py_XINCREF(ret);
	return ret;
}

int parse_byteorder(const char *s, bool *ret)
{
	if (strcmp(s, "little") == 0) {
		*ret = true;
		return 0;
	} else if (strcmp(s, "big") == 0) {
		*ret = false;
		return 0;
	} else {
		PyErr_SetString(PyExc_ValueError,
				"byteorder must be either 'little' or 'big'");
		return -1;
	}
}

int parse_optional_byteorder(PyObject *obj, enum drgn_byte_order *ret)
{
	if (obj == Py_None) {
		*ret = DRGN_PROGRAM_ENDIAN;
		return 0;
	}
	if (PyUnicode_Check(obj)) {
		const char *s;

		s = PyUnicode_AsUTF8(obj);
		if (strcmp(s, "little") == 0) {
			*ret = DRGN_LITTLE_ENDIAN;
			return 0;
		} else if (strcmp(s, "big") == 0) {
			*ret = DRGN_BIG_ENDIAN;
			return 0;
		}
	}
	PyErr_SetString(PyExc_ValueError,
			"byteorder must be 'little', 'big', or None");
	return -1;
}

int path_converter(PyObject *o, void *p)
{
	struct path_arg *path = p;
	int is_bytes, is_unicode;
	PyObject *bytes = NULL;
	Py_ssize_t length = 0;
	char *tmp;

	if (o == NULL) {
		path_cleanup(p);
		return 1;
	}

	path->object = path->cleanup = NULL;
	Py_INCREF(o);

	if (path->allow_none && o == Py_None) {
		path->path = NULL;
		path->length = 0;
		goto out;
	}

	is_bytes = PyBytes_Check(o);
	is_unicode = PyUnicode_Check(o);

	if (!is_bytes && !is_unicode) {
		_Py_IDENTIFIER(__fspath__);
		PyObject *func;

		func = _PyObject_LookupSpecial(o, &PyId___fspath__);
		if (func == NULL)
			goto err_format;
		Py_DECREF(o);
		o = PyObject_CallObject(func, NULL);
		Py_DECREF(func);
		if (o == NULL)
			return 0;
		is_bytes = PyBytes_Check(o);
		is_unicode = PyUnicode_Check(o);
	}

	if (is_unicode) {
		if (!PyUnicode_FSConverter(o, &bytes))
			goto err;
	} else if (is_bytes) {
		bytes = o;
		Py_INCREF(bytes);
	} else {
err_format:
		PyErr_Format(PyExc_TypeError,
			     "expected string, bytes, or os.PathLike, not %s",
			     Py_TYPE(o)->tp_name);
		goto err;
	}

	length = PyBytes_GET_SIZE(bytes);
	tmp = PyBytes_AS_STRING(bytes);
	if ((size_t)length != strlen(tmp)) {
		PyErr_SetString(PyExc_TypeError,
				"path has embedded nul character");
		goto err;
	}

	path->path = tmp;
	if (bytes == o)
		Py_DECREF(bytes);
	else
		path->cleanup = bytes;
	path->length = length;

out:
	path->object = o;
	return Py_CLEANUP_SUPPORTED;

err:
	Py_XDECREF(o);
	Py_XDECREF(bytes);
	return 0;
}

void path_cleanup(struct path_arg *path)
{
	Py_CLEAR(path->object);
	Py_CLEAR(path->cleanup);
}

int enum_converter(PyObject *o, void *p)
{
	struct enum_arg *arg = p;

	if (arg->allow_none && o == Py_None)
		return 1;

	if (!PyObject_TypeCheck(o, (PyTypeObject *)arg->type)) {
		PyErr_Format(PyExc_TypeError,
			     "expected %s%s, not %s",
			     ((PyTypeObject *)arg->type)->tp_name,
			     arg->allow_none ? " or None" : "",
			     Py_TYPE(o)->tp_name);
		return 0;
	}

	o = PyObject_GetAttrString(o, "value");
	if (!o)
		return 0;

	arg->value = PyLong_AsUnsignedLong(o);
	Py_DECREF(o);
	if (arg->value == -1 && PyErr_Occurred())
		return 0;
	return 1;
}
