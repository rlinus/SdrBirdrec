/*! \file class_handle.h
This file provides methods to wrap a C++ class in a MATLAB class, via a mex interface, safely, without memory leaks, whilst achieving 
an interface in MATLAB that is similar to the underlying C++ interface. 

Source: https://ch.mathworks.com/matlabcentral/fileexchange/38964-example-matlab-class-wrapper-for-a-c-class?s_tid=FX_rc2_behav
*/
#pragma once

#include <mex.h>
#include <cstdint>
#include <string>
#include <cstring>
#include <typeinfo>

#define CLASS_HANDLE_SIGNATURE 0xFF00F0A5
template<class base> class class_handle
{
public:
    class_handle(base *ptr) : ptr_m(ptr), name_m(typeid(base).name()) { signature_m = CLASS_HANDLE_SIGNATURE; }
    ~class_handle() { signature_m = 0; delete ptr_m; }
    bool isValid() { return ((signature_m == CLASS_HANDLE_SIGNATURE) && !strcmp(name_m.c_str(), typeid(base).name())); }
    base *ptr() { return ptr_m; }

private:
    uint32_t signature_m;
    std::string name_m;
    base *ptr_m;
};

template<class base> inline mxArray *convertPtr2Mat(base *ptr)
{
    mexLock();
    mxArray *out = mxCreateNumericMatrix(1, 1, mxUINT64_CLASS, mxREAL);
    *((uint64_t *)mxGetData(out)) = reinterpret_cast<uint64_t>(new class_handle<base>(ptr));
    return out;
}

template<class base> inline class_handle<base> *convertMat2HandlePtr(const mxArray *in)
{
    if (mxGetNumberOfElements(in) != 1 || mxGetClassID(in) != mxUINT64_CLASS || mxIsComplex(in))
        mexErrMsgTxt("Input must be a real uint64 scalar.");
    class_handle<base> *ptr = reinterpret_cast<class_handle<base> *>(*((uint64_t *)mxGetData(in)));
    if (!ptr->isValid())
        mexErrMsgTxt("Handle not valid.");
    return ptr;
}

template<class base> inline base *convertMat2Ptr(const mxArray *in)
{
    return convertMat2HandlePtr<base>(in)->ptr();
}

template<class base> inline void destroyObject(const mxArray *in)
{
    delete convertMat2HandlePtr<base>(in);
    mexUnlock();
}

